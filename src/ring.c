#include <math.h>
#include <stdlib.h>

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
    if (n > r->len)
        n = r->len;
    if (n == 0)
        return NAN;

    double sum = 0.0;
    for (size_t i = r->len - n; i < r->len; i++)
        sum += ring_at(r, i);
    return sum / (double)n;
}

double ring_min(const ring_t *r, size_t n)
{
    if (n > r->len)
        n = r->len;
    if (n == 0)
        return NAN;
    double m = INFINITY;
    for (size_t i = r->len - n; i < r->len; i++)
        if (ring_at(r, i) < m)
            m = ring_at(r, i);
    return m;
}

double ring_max(const ring_t *r, size_t n)
{
    if (n > r->len)
        n = r->len;
    if (n == 0)
        return NAN;
    double m = -INFINITY;
    for (size_t i = r->len - n; i < r->len; i++)
        if (ring_at(r, i) > m)
            m = ring_at(r, i);
    return m;
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

double ring_percentile(const ring_t *r, size_t n, double p)
{
    double tmp[RING_CAP];

    if (n > r->len)
        n = r->len;
    if (n == 0)
        return NAN;
    for (size_t i = 0; i < n; i++)
        tmp[i] = ring_at(r, r->len - n + i);
    qsort(tmp, n, sizeof tmp[0], cmp_double);

    double exact = p / 100.0 * (double)n;
    size_t rank = (size_t)exact;
    if ((double)rank < exact)
        rank++;
    if (rank < 1)
        rank = 1;
    if (rank > n)
        rank = n;
    return tmp[rank - 1];
}
