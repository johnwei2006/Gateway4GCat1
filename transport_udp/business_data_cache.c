#include "business_data_cache.h"

/**
 * @brief 缓存初始化（系统启动时调用一次）
 */
rt_err_t business_data_cache_init(void)
{
    // 初始化互斥锁
    if (rt_mutex_init(&cache_mutex, "bus_cache_mutex", RT_IPC_FLAG_FIFO) != RT_EOK)
    {
        return RT_ERROR;
    }

    // 清空缓存表
    memset(g_cache_table, 0, sizeof(g_cache_table));
    return RT_EOK;
}

/**
 * @brief 查找缓存节点（内部函数）
 */
static cache_node_t *cache_find_node(uint32_t event_id)
{
    for (int i = 0; i < CACHE_MAX_COUNT; i++)
    {
        if (g_cache_table[i].used && g_cache_table[i].event_id == event_id)
        {
            return &g_cache_table[i];
        }
    }
    return RT_NULL;
}

/**
 * @brief 申请空闲节点（内部函数）
 */
static cache_node_t *cache_alloc_node(void)
{
    for (int i = 0; i < CACHE_MAX_COUNT; i++)
    {
        if (!g_cache_table[i].used)
        {
            g_cache_table[i].used = RT_TRUE;
            return &g_cache_table[i];
        }
    }
    return RT_NULL;
}

static int cache_used_size(void)
{
    int count = 0;
    for (int i = 0; i < CACHE_MAX_COUNT; i++)
    {
        if (g_cache_table[i].used)
        {
            count++;
        }
    }
    return count;
}
// ==================== 你要的三个接口 ====================

/**
 * @brief 存入/更新缓存（存在则覆盖，不存在则新增）
 * @param event_id      事件ID（key）
 * @param business_data 业务数据（value）
 * @return RT_EOK 成功
 */
rt_err_t put_business_data_cache(uint32_t event_id, void *business_data)
{
    if (business_data == RT_NULL)
        return -RT_EINVAL;

    rt_mutex_take(&cache_mutex, RT_WAITING_FOREVER);

    // 1. 查找是否已存在
    cache_node_t *node = cache_find_node(event_id);

    // 2. 存在 → 覆盖
    if (node != RT_NULL)
    {
        if (node->business_data != RT_NULL)
        {
            rt_free(node->business_data);  // 释放旧数据
        }
        node->business_data = business_data;
        rt_mutex_release(&cache_mutex);
        return RT_EOK;
    }

    // 3. 不存在 → 新建
    node = cache_alloc_node();
    if (node == RT_NULL)
    {
        rt_mutex_release(&cache_mutex);
        return -RT_ENOMEM;  // 缓存满了
    }

    node->event_id = event_id;
    node->business_data = business_data;
    
    int used_size = cache_used_size();
    rt_kprintf("put_business_data_cache: used_size: %d \r\n", used_size);
    
    rt_mutex_release(&cache_mutex);
    return RT_EOK;
}

/**
 * @brief 根据 event_id 获取缓存数据
 * @param event_id      事件ID
 * @param business_data 输出：数据指针（你自己定义的结构体）
 * @return RT_EOK 成功
 */
rt_err_t get_business_data_cache(uint32_t event_id, void **business_data)
{
    // if (*business_data == RT_NULL)
    //     return -RT_EINVAL;

    rt_mutex_take(&cache_mutex, RT_WAITING_FOREVER);

    cache_node_t *node = cache_find_node(event_id);
    if (node == RT_NULL)
    {
        rt_mutex_release(&cache_mutex);
        return RT_ERROR;  // 未找到
    }

    *business_data = node->business_data;  // 返回数据指针

    rt_mutex_release(&cache_mutex);
    return RT_EOK;
}

/**
 * @brief 删除缓存（自动释放内存）
 * @param event_id 事件ID
 * @return RT_EOK 成功
 */
rt_err_t remove_business_data_cache(uint32_t event_id)
{
    rt_mutex_take(&cache_mutex, RT_WAITING_FOREVER);

    cache_node_t *node = cache_find_node(event_id);
    if (node == RT_NULL)
    {
        rt_mutex_release(&cache_mutex);
        return RT_ERROR;
    }

    // 释放业务数据内存
    if (node->business_data != RT_NULL)
    {
        rt_free(node->business_data);
        node->business_data = RT_NULL;
    }

    // 清空节点
    node->used = RT_FALSE;
    node->event_id = 0;

    rt_mutex_release(&cache_mutex);
    return RT_EOK;
}

/**
 * @brief 清空所有缓存（可选）
 */
void clear_all_business_cache(void)
{
    rt_mutex_take(&cache_mutex, RT_WAITING_FOREVER);

    for (int i = 0; i < CACHE_MAX_COUNT; i++)
    {
        if (g_cache_table[i].used && g_cache_table[i].business_data)
        {
            rt_free(g_cache_table[i].business_data);
        }
    }

    memset(g_cache_table, 0, sizeof(g_cache_table));

    rt_mutex_release(&cache_mutex);
}

INIT_COMPONENT_EXPORT(business_data_cache_init);