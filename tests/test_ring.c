#include "ring.h"
#include "test.h"

static ring_t r; /* RING_CAP doubles: keep it off the stack */

int main(void)
{
    CHECK(ring_len(&r) == 0);
    CHECK(isnan(ring_last(&r)));
    CHECK(isnan(ring_avg(&r, 10)));

    for (int i = 1; i <= 3; i++)
        ring_push(&r, i);
    CHECK(ring_len(&r) == 3);
    CHECK(ring_at(&r, 0) == 1);
    CHECK(ring_last(&r) == 3);
    CHECK_NEAR(ring_avg(&r, 2), 2.5, 1e-9);
    /* asking for more samples than we have averages what we have */
    CHECK_NEAR(ring_avg(&r, 100), 2.0, 1e-9);
    CHECK(isnan(ring_at(&r, 3)));
    CHECK(ring_min(&r, 3) == 1);
    CHECK(ring_max(&r, 2) == 3);
    CHECK(isnan(ring_min(&r, 0)));
    CHECK(ring_percentile(&r, 3, 50) == 2);
    CHECK(ring_percentile(&r, 3, 100) == 3);
    CHECK(ring_percentile(&r, 3, 0) == 1);

    /* wrap around: only the newest RING_CAP samples survive */
    for (int i = 4; i <= RING_CAP + 10; i++)
        ring_push(&r, i);
    CHECK(ring_len(&r) == RING_CAP);
    CHECK(ring_at(&r, 0) == 11);
    CHECK(ring_last(&r) == RING_CAP + 10);

    return test_report("ring");
}
