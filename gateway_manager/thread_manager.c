#include <rtthread.h>
#include "thread_manager.h"

rt_inline void object_split(int len)
{
    while (len--) rt_kprintf("-");
}

typedef struct
{
    rt_list_t *list;
    rt_list_t **array;
    rt_uint8_t type;
    int nr;             /* input: max nr, can't be 0 */
    int nr_out;         /* out: got nr */
} list_get_next_t;

static void list_find_init(list_get_next_t *p, rt_uint8_t type, rt_list_t **array, int nr)
{
    struct rt_object_information *info;
    rt_list_t *list;

    info = rt_object_get_information((enum rt_object_class_type)type);
    list = &info->object_list;

    p->list = list;
    p->type = type;
    p->array = array;
    p->nr = nr;
    p->nr_out = 0;
}

static rt_list_t *list_get_next(rt_list_t *current, list_get_next_t *arg)
{
    int first_flag = 0;
    rt_ubase_t level;
    rt_list_t *node, *list;
    rt_list_t **array;
    int nr;

    arg->nr_out = 0;

    if (!arg->nr || !arg->type)
    {
        return (rt_list_t *)RT_NULL;
    }

    list = arg->list;

    if (!current) /* find first */
    {
        node = list;
        first_flag = 1;
    }
    else
    {
        node = current;
    }

    level = rt_hw_interrupt_disable();

    if (!first_flag)
    {
        struct rt_object *obj;
        /* The node in the list? */
        obj = rt_list_entry(node, struct rt_object, list);
        if ((obj->type & ~RT_Object_Class_Static) != arg->type)
        {
            rt_hw_interrupt_enable(level);
            return (rt_list_t *)RT_NULL;
        }
    }

    nr = 0;
    array = arg->array;
    while (1)
    {
        node = node->next;

        if (node == list)
        {
            node = (rt_list_t *)RT_NULL;
            break;
        }
        nr++;
        *array++ = node;
        if (nr == arg->nr)
        {
            break;
        }
    }

    rt_hw_interrupt_enable(level);
    arg->nr_out = nr;
    return node;
}
void print_all_thread_stack(void)
{
    rt_ubase_t level;
    list_get_next_t find_arg;
    rt_list_t *obj_list[8];
    rt_list_t *next = (rt_list_t*)RT_NULL;
    const char *item_title = "thread";
    int maxlen;

    list_find_init(&find_arg, RT_Object_Class_Thread, obj_list, sizeof(obj_list)/sizeof(obj_list[0]));

    maxlen = RT_NAME_MAX;

    rt_kprintf("%-*.s pri  status      sp     stack size max used left tick  error\n", maxlen, item_title); object_split(maxlen);
    rt_kprintf(     " ---  ------- ---------- ----------  ------  ---------- ---\n");

    do
    {
        next = list_get_next(next, &find_arg);
        {
            int i;
            for (i = 0; i < find_arg.nr_out; i++)
            {
                struct rt_object *obj;
                struct rt_thread thread_info, *thread;

                obj = rt_list_entry(obj_list[i], struct rt_object, list);
                level = rt_hw_interrupt_disable();

                if ((obj->type & ~RT_Object_Class_Static) != find_arg.type)
                {
                    rt_hw_interrupt_enable(level);
                    continue;
                }
                /* copy info */
                memcpy(&thread_info, obj, sizeof thread_info);
                rt_hw_interrupt_enable(level);

                thread = (struct rt_thread*)obj;
                {
                    rt_uint8_t stat;
                    rt_uint8_t *ptr;

                    rt_kprintf("%-*.*s %3d ", maxlen, 30, thread->name, thread->current_priority);
                    stat = (thread->stat & RT_THREAD_STAT_MASK);
                    if (stat == RT_THREAD_READY)        rt_kprintf(" ready  ");
                    else if (stat == RT_THREAD_SUSPEND) rt_kprintf(" suspend");
                    else if (stat == RT_THREAD_INIT)    rt_kprintf(" init   ");
                    else if (stat == RT_THREAD_CLOSE)   rt_kprintf(" close  ");
                    else if (stat == RT_THREAD_RUNNING) rt_kprintf(" running");

                    ptr = (rt_uint8_t *)thread->stack_addr;
                    while (*ptr == '#')ptr ++;

                    rt_kprintf(" 0x%08x 0x%08x    %02d%%   0x%08x %03d\n",
                            thread->stack_size + ((rt_ubase_t)thread->stack_addr - (rt_ubase_t)thread->sp),
                            thread->stack_size,
                            (thread->stack_size - ((rt_ubase_t) ptr - (rt_ubase_t) thread->stack_addr)) * 100
                            / thread->stack_size,
                            thread->remaining_tick,
                            thread->error);
                }
            }
        }
    }
    while (next != (rt_list_t*)RT_NULL);
}

#define MAX_THREAD_RECORD_NUM  20   // 最大记录线程数量
#define SCAN_PERIOD_MS          1000  // 巡检周期ms
#define SCAN_THR_STACK          512  // 监控线程栈大小
#define SCAN_THR_PRIO           30    // 监控线程优先级
#define STACK_WARN_RATIO        0.8f  // 栈占用告警阈值

// RT-Thread存在两种栈魔数，同时兼容
#define RT_STACK_MAGIC_OLD 0xdeadbeef
#define RT_STACK_MAGIC_NEW 0x55aa55aa

/**
 * @brief 手动计算线程空闲栈大小（兼容两种内核魔数）
 * @param tid 线程句柄
 * @return 空闲栈字节数
 */
static rt_size_t thread_calc_free_stack(rt_thread_t tid)
{
    if (tid == RT_NULL || tid->stack_addr == RT_NULL || tid->stack_size == 0)
        return 0;

    rt_uint32_t *stack_base = (rt_uint32_t *)tid->stack_addr;
    rt_size_t word_cnt = tid->stack_size / sizeof(rt_uint32_t);
    rt_size_t free_word = 0;

    // 从栈底低地址向上遍历，连续魔数 = 空闲栈
    for (; free_word < word_cnt; free_word++)
    {
        rt_uint32_t val = stack_base[free_word];
        // 兼容新旧两种魔数
        if (val != RT_STACK_MAGIC_OLD && val != RT_STACK_MAGIC_NEW)
            break;
    }
    return free_word * sizeof(rt_uint32_t);
}

/* 根据线程句柄查找记录，无则新建 */
static thread_stack_record_t *find_or_create_record(rt_thread_t tid)
{
    rt_uint8_t i;
    for (i = 0; i < g_record_cnt; i++)
    {
        if (g_stack_records[i].tid == tid)
        {
            return &g_stack_records[i];
        }
    }

    if (g_record_cnt >= MAX_THREAD_RECORD_NUM)
    {
        return RT_NULL;
    }

    thread_stack_record_t *rec = &g_stack_records[g_record_cnt];
    rec->tid = tid;
    rec->stack_total = tid->stack_size;
    rec->max_used_record = 0;
    rec->peak_tick = 0;
    g_record_cnt++;
    return rec;
}

/* 遍历所有线程，更新栈峰值与时间戳 RT4 通用 */
static void scan_and_update_stack_peak(void)
{
    struct rt_object_information *obj_info;
    struct rt_list_node *node;
    rt_thread_t tid;

    obj_info = rt_object_get_information(RT_Object_Class_Thread);
    if (obj_info == RT_NULL)
        return;

    rt_enter_critical();
    rt_list_for_each(node, &obj_info->object_list)
    {
        tid = (rt_thread_t)rt_list_entry(node, struct rt_object, list);
        if (tid == RT_NULL)
            continue;

        rt_size_t free_stack = thread_calc_free_stack(tid);
        rt_size_t cur_max_used = tid->stack_size - free_stack;

        thread_stack_record_t *rec = find_or_create_record(tid);
        if (rec == RT_NULL)
            continue;

        // 刷新更大峰值，同步记录当前tick
        if (cur_max_used > rec->max_used_record)
        {
            rec->max_used_record = cur_max_used;
            rec->peak_tick = rt_tick_get();

            float usage_rate = (float)cur_max_used / rec->stack_total;
            if (usage_rate >= STACK_WARN_RATIO)
            {
                rt_kprintf("[STACK WARN] Thread:%s MaxUsed:%d Total:%d Tick:%d Rate:%.2f\n",
                           tid->name, cur_max_used, rec->stack_total, rec->peak_tick, usage_rate);
            }
        }
    }
    rt_exit_critical();
}

/* FinSH打印全部线程栈峰值+峰值时间 */
void print_all_stack_record(void)
{
    rt_kprintf("================ Thread Stack Peak Record ================\n");
    rt_kprintf("%-12s %-8s %-10s %-12s\n", "Name", "Total", "MaxUsed", "PeakTick");
    for (rt_uint8_t i = 0; i < g_record_cnt; i++)
    {
        thread_stack_record_t *rec = &g_stack_records[i];
        rt_kprintf("%-12s %-8d %-10d %-12d\n",
                   rec->tid->name,
                   rec->stack_total,
                   rec->max_used_record,
                   rec->peak_tick);
    }
    rt_kprintf("==========================================================\n");
}

/* 后台监控线程入口 */
static void stack_monitor_entry(void *param)
{
    while (1)
    {
        rt_thread_mdelay(SCAN_PERIOD_MS);
        scan_and_update_stack_peak();
    }
}

/* 自动初始化监控线程 */
static int stack_monitor_init(void)
{
    rt_thread_t monitor_tid = rt_thread_create(
        "stackmon",
        stack_monitor_entry,
        RT_NULL,
        SCAN_THR_STACK,
        SCAN_THR_PRIO,
        10
    );
    if (monitor_tid != RT_NULL)
    {
        rt_thread_startup(monitor_tid);
        rt_kprintf("Stack monitor thread start, scan period: %d ms\n", SCAN_PERIOD_MS);
    }
    else
    {
        rt_kprintf("Create stack monitor thread failed!\n");
    }
    return 0;
}
//INIT_APP_EXPORT(stack_monitor_init);