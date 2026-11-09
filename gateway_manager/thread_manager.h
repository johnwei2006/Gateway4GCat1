#ifndef _THREAD_MANAGER_H_
#define _THREAD_MANAGER_H_

void print_all_thread_stack(void);

/* 单线程栈峰值记录结构体 */
typedef struct
{
    rt_thread_t tid;               // 线程句柄
    rt_size_t stack_total;         // 总栈大小
    rt_size_t max_used_record;     // 记录到的最大栈占用
    rt_tick_t peak_tick;           // 峰值出现时的系统tick
} thread_stack_record_t;

#define MAX_THREAD_RECORD_NUM 32    // 最大支持记录32个线程

static thread_stack_record_t g_stack_records[MAX_THREAD_RECORD_NUM];
static rt_uint8_t g_record_cnt = 0;
static thread_stack_record_t *find_or_create_record(rt_thread_t tid);
static void scan_and_update_stack_peak(void);
void print_all_stack_record(void);
static void stack_monitor_entry(void *param);
static int stack_monitor_init(void);
#endif /*_THREAD_MANAGER_H_*/
