#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h>

#define list_entry(ptr, type, member)   \
        container_of(ptr, type, member)

#define list_for_each_entry(cursor, head, member)   \
        for(cursor = list_entry((head)->next, typeof(*cursor), member);    \
            &cursor->member != (head);  \
            cursor = list_entry(cursor->member.next, typeof(*cursor), member))

struct list_head {
    struct list_head *prev, *next;
};

static inline void list_head_init(struct list_head *head)
{
    head->next = head->prev = head;
}

static inline int list_empty(struct list_head *head)
{
    return (head->next == head) || (head->prev == head);
}

static inline void list_del(struct list_head *node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
}

static inline struct list_head *list_pop(struct list_head *head)
{
    struct list_head *ret_node = head->next;
    list_del(ret_node);
    return ret_node;
}

static inline void __list_add(struct list_head *node,
                              struct list_head *prev,
                              struct list_head *next)
{
    node->next = next;
    node->prev = prev;
    prev->next = node;
    next->prev = node;
}

static inline void list_add(struct list_head *node, struct list_head *head)
{
    __list_add(node, head, head->next);
}

static inline void list_add_tail(struct list_head *node, struct list_head *head)
{
    __list_add(node, head->prev, head);
}

#endif