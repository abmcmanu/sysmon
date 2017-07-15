#include <math.h>

#include "ring.h"

void ring_push(ring_t *r, double x)
{
    r->v[r->head] = x;
    r->head = (r->head + 1) % RING_CAP;
    if (r->len < RING_CAP)
        r->len++;
}

size_t ring_len(const ring_t *r)
{
    return r->len;
}

double ring_at(const ring_t *r, size_t i)
{
    if (i >= r->len)
        return NAN;
    size_t start = (r->head + RING_CAP - r->len) % RING_CAP;
    return r->v[(start + i) % RING_CAP];
}

double ring_last(const ring_t *r)
{
    return r->len ? ring_at(r, r->len - 1) : NAN;
}

double ring_avg(const ring_t *r, size_t n)
{
    if (n == 0)
        return NAN;

    double sum = 0.0;
    for (size_t i = r->len - n; i < r->len; i++)
        sum += ring_at(r, i);
    return sum / (double)n;
}
