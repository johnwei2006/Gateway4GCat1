#ifndef CPS_LIST_H_
#define CPS_LIST_H_

#include <stdio.h>
#include <stdlib.h>

typedef struct cps_list_node
{
    void *data;  
    struct cps_list_node *next;
} cps_list_node_t;

/* 初始化空链表（返回NULL） */
cps_list_node_t* cps_list_init(void);

/* 向链表尾部添加节点 */
void cps_list_add(cps_list_node_t **head, void *data);

/* 获取链表长度 */
int cps_list_size(cps_list_node_t *head);

/* 按索引获取节点数据（index从1开始） */
void* cps_list_get_node(cps_list_node_t *head, int index);

/* 按索引删除节点（返回被删除节点的data，由用户管理） */
cps_list_node_t* cps_list_delete(cps_list_node_t **head, int index);

/* 释放整个链表（仅释放节点，不释放data） */
void cps_list_free(cps_list_node_t *head);

#endif /* CPS_LIST_H_ */