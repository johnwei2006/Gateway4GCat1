#include "cps_list.h"
#include <rtthread.h>

cps_list_node_t* cps_list_init()
{
    return NULL; // 空链表初始化为NULL（无哨兵节点）
}

void cps_list_add(cps_list_node_t **head, void *data) {
    //int list_size = cps_list_size(*head);
    //rt_kprintf("cps_list_add before size: %d\r\n ", list_size);
    if (head == NULL || data == NULL) {
        return;
    }

    // 创建新节点
    cps_list_node_t *pNode = (cps_list_node_t *)rt_malloc(sizeof(cps_list_node_t));
    if (pNode == RT_NULL) {
        rt_kprintf("malloc failed\n");
        return;
    }
    pNode->data = data;
    pNode->next = NULL;

    // 空链表：新节点作为头节点
    if (*head == NULL) {
        *head = pNode;
        return;
    }

    // 非空链表：添加到尾部
    cps_list_node_t *p1 = *head;
    while (p1->next != NULL) {
        p1 = p1->next;
    }
    p1->next = pNode;
    //list_size = cps_list_size(*head);
    //rt_kprintf("cps_list_add after size: %d\r\n ", list_size);
}

int cps_list_size(cps_list_node_t *head)
{
    int nCount = 0;
    cps_list_node_t *p1 = head;
    while (p1 != NULL) { // 直接按节点存在性计数
        nCount++;
        p1 = p1->next;
    }
    return nCount; // 空链表返回0
}

void* cps_list_get_node(cps_list_node_t *head, int index)
{
    if (head == NULL || index <= 0) {
        return NULL;
    }

    int nCount = 1;
    cps_list_node_t *p1 = head;
    while (p1 != NULL) {
        if (nCount == index) {
            return p1->data;
        }
        p1 = p1->next;
        nCount++;
    }
    return NULL; // 索引超出范围
}

cps_list_node_t* cps_list_delete(cps_list_node_t **head, int index)
{
    // 入参合法性检查
    if (head == NULL || *head == NULL || index <= 0) {
        printf("invalid input\n");
        return NULL;
    }

    // 检查索引是否越界
    int len = cps_list_size(*head);
    if (index > len) {
        printf("index out of range\n");
        return NULL;
    }

    cps_list_node_t *deleted = NULL;

    // ==============================================
    // 情况1：删除第一个节点
    // ==============================================
    if (index == 1) {
        deleted = *head;
        *head = (*head)->next;  // 头指针后移
        
        deleted->next = NULL;   // 【修复点】断开被删除节点的指针
        return deleted;
    }

    // ==============================================
    // 情况2：删除中间节点 / 尾节点
    // ==============================================
    int count = 1;
    cps_list_node_t *prev = *head;

    // 找到待删除节点的**前一个节点**
    while (count < index - 1) {
        prev = prev->next;
        count++;
    }

    deleted = prev->next;           // 待删除节点
    prev->next = deleted->next;     // 前驱跳过删除节点
    deleted->next = NULL;           // 【修复点】关键！删除节点必须断链

    return deleted;  // 现在一定能返回正确的节点指针
}

void cps_list_free(cps_list_node_t *head)
{
    cps_list_node_t *ptr = head;
    while (ptr != RT_NULL) {
        cps_list_node_t *next = ptr->next; // 提前保存下一个节点
        rt_free(ptr->data);
        rt_free(ptr); // 仅释放节点，不释放data（用户管理）
        ptr = next;
    }
}