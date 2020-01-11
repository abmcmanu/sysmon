#include "ring.h"
#include "test.h"

static ring_t r; /* RING_CAP doubles: keep it off the stack */

int main(void)
{
    CHECK(ring_len(&r) == 0);
    CHECK(isnan(ring_last(&r)));

    for (int i = 1; i <= 3; i++)
        ring_push(&r, i);
    CHECK(ring_len(&r) == 3);
    CHECK(ring_at(&r, 0) == 1);
    CHECK(ring_last(&r) == 3);
    CHECK_NEAR(ring_avg(&r, 2), 2.5, 1e-9);
    CHECK(isnan(ring_at(&r, 3)));

    /* wrap around: only the newest RING_CAP samples survive */
    for (int i = 4; i <= RING_CAP + 10; i++)
        ring_push(&r, i);
    CHECK(ring_len(&r) == RING_CAP);
    CHECK(ring_at(&r, 0) == 11);
    CHECK(ring_last(&r) == RING_CAP + 10);

    return test_report("ring");
}
