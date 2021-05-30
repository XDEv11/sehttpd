#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include "http.h"
#include "timeout/timeout.h"

#define TIMEOUT_DEFAULT 500 /* ms */

typedef int (*timer_callback)(http_request_t *req);

int timer_init();
int find_timer();
void time_elapse(int);
void handle_expired_timers();

void add_timer(http_request_t *req, size_t timeout, timer_callback cb);
void del_timer(http_request_t *req);

#endif
