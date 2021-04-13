#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "logger.h"
#include "timer.h"

#define TIMER_INFINITE (-1)
#define PQ_DEFAULT_SIZE 10

typedef bool (*prio_queue_comparator)(void *pi, void *pj);

/* priority queue with binary heap */
typedef struct {
    void **priv;
    size_t size;
    size_t capacity;
    prio_queue_comparator comp;
} prio_queue_t;

static bool prio_queue_init(prio_queue_t *ptr,
                            prio_queue_comparator comp,
                            size_t capacity)
{
    ptr->priv = malloc(sizeof(void *) * capacity);
    if (!ptr->priv) {
        log_err("prio_queue_init: malloc failed");
        return false;
    }

    ptr->size = 0;
    ptr->capacity = capacity;
    ptr->comp = comp;
    return true;
}

static inline bool prio_queue_is_empty(prio_queue_t *ptr)
{
    return ptr->size == 0;
}

static inline size_t prio_queue_size(prio_queue_t *ptr)
{
    return ptr->size;
}

static inline void *prio_queue_min(prio_queue_t *ptr)
{
    return prio_queue_is_empty(ptr) ? NULL : ptr->priv[0];
}

static bool resize(prio_queue_t *ptr, size_t new_capacity)
{
    if (new_capacity < ptr->size) {
        log_err("resize: new_capacity to small");
        return false;
    }

    /* TODO: use memory pool to avoid unexpected fragmentation */
    void **new_ptr = malloc(sizeof(void *) * new_capacity);
    if (!new_ptr) {
        log_err("resize: malloc failed");
        return false;
    }

    memcpy(new_ptr, ptr->priv, sizeof(void *) * ptr->size);
    free(ptr->priv);
    ptr->priv = new_ptr;
    ptr->capacity = new_capacity;
    return true;
}

static inline void swap(prio_queue_t *ptr, size_t i, size_t j)
{
    void *tmp = ptr->priv[i];
    ptr->priv[i] = ptr->priv[j];
    ptr->priv[j] = tmp;
}

static inline void swim(prio_queue_t *ptr, size_t k)
{
    while (k > 0 && ptr->comp(ptr->priv[k], ptr->priv[(k - 1) >> 1])) {
        swap(ptr, k, (k - 1) >> 1);
        k = (k - 1) >> 1;
    }
}

static inline size_t sink(prio_queue_t *ptr, size_t k)
{
    while ((k << 1) + 1 < ptr->size) {
        size_t j = (k << 1) + 1;
        if (j + 1 < ptr->size && ptr->comp(ptr->priv[j + 1], ptr->priv[j]))
            ++j;
        if (!ptr->comp(ptr->priv[j], ptr->priv[k]))
            break;
        swap(ptr, j, k);
        k = j;
    }

    return k;
}

/* remove the item with minimum key value from the heap */
static bool prio_queue_delmin(prio_queue_t *ptr)
{
    if (prio_queue_is_empty(ptr))
        return true;

    swap(ptr, 0, ptr->size);
    --ptr->size;
    sink(ptr, 0);
    if (ptr->size > 0 && ptr->size <= ptr->capacity / 4) {
        if (!resize(ptr, ptr->capacity / 2))
            return false;
    }
    return true;
}

/* add a new item to the heap */
static bool prio_queue_insert(prio_queue_t *ptr, void *item)
{
    if (ptr->size == ptr->capacity) {
        if (!resize(ptr, ptr->capacity * 2))
            return false;
    }

    ptr->priv[ptr->size] = item;
    swim(ptr, ptr->size);
    ++ptr->size;
    return true;
}

static bool timer_comp(void *ti, void *tj)
{
    return ((timer_node *) ti)->key < ((timer_node *) tj)->key;
}

static prio_queue_t timer;
static size_t current_msec;

static void time_update()
{
    struct timeval tv;
    int rc UNUSED = gettimeofday(&tv, NULL);
    assert(rc == 0 && "time_update: gettimeofday error");
    current_msec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int timer_init()
{
    bool ret UNUSED = prio_queue_init(&timer, timer_comp, PQ_DEFAULT_SIZE);
    assert(ret && "prio_queue_init error");

    // time_update();
    return 0;
}

int find_timer()
{
    int time = TIMER_INFINITE;

    while (!prio_queue_is_empty(&timer)) {
        time_update();
        timer_node *node = prio_queue_min(&timer);
        assert(node && "prio_queue_min error");

        if (node->deleted) {
            bool ret UNUSED = prio_queue_delmin(&timer);
            assert(ret && "prio_queue_delmin");
            free(node);
            continue;
        }

        time = (int) (node->key - current_msec);
        time = (time > 0 ? time : 0);
        break;
    }

    return time;
}

void handle_expired_timers()
{
    bool ret UNUSED;

    while (!prio_queue_is_empty(&timer)) {
        debug("handle_expired_timers, size = %zu", prio_queue_size(&timer));
        time_update();
        timer_node *node = prio_queue_min(&timer);
        assert(node && "prio_queue_min error");

        if (node->deleted) {
            ret = prio_queue_delmin(&timer);
            assert(ret && "handle_expired_timers: prio_queue_delmin error");
            free(node);
            continue;
        }

        if (node->key > current_msec)
            return;
        if (node->callback)
            node->callback(node->request);

        ret = prio_queue_delmin(&timer);
        assert(ret && "handle_expired_timers: prio_queue_delmin error");
        free(node);
    }
}

void add_timer(http_request_t *req, size_t timeout, timer_callback cb)
{
    timer_node *node = malloc(sizeof(timer_node));
    assert(node && "add_timer: malloc error");

    time_update();
    req->timer = node;
    node->key = current_msec + timeout;
    node->deleted = false;
    node->callback = cb;
    node->request = req;

    bool ret UNUSED = prio_queue_insert(&timer, node);
    assert(ret && "add_timer: prio_queue_insert error");
}

void del_timer(http_request_t *req)
{
    // time_update();
    timer_node *node = req->timer;
    assert(node && "del_timer: req->timer is NULL");

    node->deleted = true;
}
