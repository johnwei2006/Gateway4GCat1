#ifndef BUSINESS_DATA_CACHE_H_
#define BUSINESS_DATA_CACHE_H_ 

#include <rtthread.h>
#include <stdint.h>
#include <string.h>

// ==================== 配置 ====================
#define CACHE_MAX_COUNT        5       // 最大缓存条数
#define BUSINESS_DATA_MAX_LEN  256     // 业务数据最大长度

// ==================== 缓存节点结构体 ====================
typedef struct {
    rt_bool_t   used;               // 是否使用
    uint32_t    event_id;           // KEY：事件ID
    void        *business_data;     // VALUE：业务数据指针
    //uint32_t    data_len;           // 数据长度（可选）
} cache_node_t;

// ==================== 全局缓存管理 ====================
static struct rt_mutex cache_mutex;
static cache_node_t g_cache_table[CACHE_MAX_COUNT];

rt_err_t business_data_cache_init(void);
static cache_node_t *cache_alloc_node(void);
static int cache_used_size(void);
rt_err_t put_business_data_cache(uint32_t event_id, void *business_data);
rt_err_t get_business_data_cache(uint32_t event_id, void **business_data);
rt_err_t remove_business_data_cache(uint32_t event_id);
void clear_all_business_cache(void);
#endif /*BUSINESS_DATA_CACHE_H_*/
