#ifndef TIMER_H
#define TIMER_H

#include <time.h>

struct timer {
    clock_t start_;
    clock_t end_;
};

void
start_timer(
    struct timer* timer
)
{
    timer->start_ = clock();
}

void
end_timer(
    struct timer* timer
)
{
    timer->end_ = clock();
}

void
reset_timer(
    struct timer* timer
)
{
    timer->start_ = 0;
    timer->end_   = 0;
}

clock_t
timer_duration(
    struct timer* timer
)
{
    if (timer->start_ > timer->end_)
        return 0;

    return timer->end_ - timer->start_;
}

#endif // !TIMER_H

