#ifndef LIST_H
#define LIST_H

#include <stddef.h>

typedef struct list_head {
    struct list_head *prev, *next;
} list_head;

#define INIT_LIST_HEAD(ptr)    \
    do {                       \
        list_head *_ptr = ptr; \
        (_ptr)->next = (_ptr); \
        (_ptr->prev) = (_ptr); \
    } while (0)

/* Insert a new entry to two consecutive entries */
static inline void __list_add(list_head *_new, list_head *prev, list_head *next)
{
    _new->next = next;
    next->prev = _new;
    prev->next = _new;
    _new->prev = prev;
}

static inline void list_add(list_head *_new, list_head *head)
{
    __list_add(_new, head, head->next);
}

static inline void list_add_tail(list_head *_new, list_head *head)
{
    __list_add(_new, head->prev, head);
}

/* Delete an entry to two consecutive entries */
static inline void __list_del(list_head *prev, list_head *next)
{
    prev->next = next;
    next->prev = prev;
}

static inline void list_del(list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline int list_empty(list_head *head)
{
    return (head->next == head) && (head->prev == head);
}

#define container_of(ptr, type, member)                      \
    ({                                                       \
        const typeof(((type *) 0)->member) *__mptr = (ptr);  \
        (type *) ((char *) __mptr - offsetof(type, member)); \
    })

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_for_each_safe(pos, n, head)                   \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

#endif
