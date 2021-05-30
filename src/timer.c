#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "logger.h"
#include "timer.h"

#include "timeout/timeout.h"

static struct timeouts *timer = NULL;

int timer_init()
{
    int error UNUSED;

    if (timer)
        timeouts_close(timer);

    timer = timeouts_open(TIMEOUT_mHZ, &error);

    assert(!error && "timeouts_open error");

    return 0;
}

int find_timer()
{
    return timeouts_timeout(timer);
}

void handle_expired_timers()
{
    for (struct timeout *to; (to = timeouts_get(timer));) {
        if (to->callback.fn)
            to->callback.fn(to->callback.arg);
        free(to);
    }
}

void add_timer(http_request_t *req, size_t timeout, timer_callback cb)
{
    struct timeout *to = malloc(sizeof(struct timeout));
    assert(to && "add_timer: malloc error");

    req->timer = to;
    timeout_init(to, 0);
    timeout_setcb(to, cb, req);

    timeouts_add(timer, to, timeout);
}

void del_timer(http_request_t *req)
{
    struct timeout *to = req->timer;
    assert(to && "del_timer: req->timer is NULL");

    timeout_del(to);
    free(to);
}
