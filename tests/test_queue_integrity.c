/*
 * S1TMS-7: queue integrity and safe operation.
 * Unit tests for the overflow policy (drop and count), empty-queue safety,
 * NULL safety and FIFO order across wrap-around.
 */
#include <stdio.h>

#include "queue.h"
#include "test_util.h"

static Vehicle make_vehicle(unsigned int id)
{
    Vehicle v;
    v.id = id;
    v.arrival_tick = id;
    return v;
}

static void test_init_clears_state(void)
{
    VehicleQueue q;

    q.front = 7;
    q.size = 9;
    q.dropped = 5;
    queue_init(&q);

    CHECK(q.front == 0);
    CHECK(q.size == 0);
    CHECK(q.dropped == 0);
    CHECK(queue_is_empty(&q));
    CHECK(!queue_is_full(&q));
}

static void test_dequeue_on_empty_is_safe(void)
{
    VehicleQueue q;
    Vehicle out = { 777, 888 };

    queue_init(&q);
    CHECK(dequeue(&q, &out) == 0);
    CHECK(out.id == 777);          /* output left untouched */
    CHECK(out.arrival_tick == 888);
    CHECK(q.size == 0);
    CHECK(q.front == 0);           /* no state corruption */
}

static void test_fifo_order(void)
{
    VehicleQueue q;
    Vehicle out;
    unsigned int i;

    queue_init(&q);
    for (i = 1; i <= 5; i++) {
        CHECK(enqueue(&q, make_vehicle(i)) == 1);
    }
    for (i = 1; i <= 5; i++) {
        CHECK(dequeue(&q, &out) == 1);
        CHECK(out.id == i);
    }
    CHECK(queue_is_empty(&q));
}

static void test_fill_to_exact_capacity_then_overflow(void)
{
    VehicleQueue q;
    Vehicle out;
    unsigned int i;

    queue_init(&q);
    for (i = 1; i <= QUEUE_CAPACITY; i++) {
        CHECK(enqueue(&q, make_vehicle(i)) == 1);   /* capacity: all fit */
    }
    CHECK(queue_is_full(&q));
    CHECK(q.size == QUEUE_CAPACITY);
    CHECK(q.dropped == 0);

    CHECK(enqueue(&q, make_vehicle(1000)) == 0);    /* capacity + 1: dropped */
    CHECK(enqueue(&q, make_vehicle(1001)) == 0);
    CHECK(q.size == QUEUE_CAPACITY);                /* unchanged */
    CHECK(q.dropped == 2);                          /* counted */

    CHECK(dequeue(&q, &out) == 1);
    CHECK(out.id == 1);                             /* oldest vehicle kept, newest dropped */
}

static void test_space_is_reusable_after_overflow(void)
{
    VehicleQueue q;
    Vehicle out;
    unsigned int i;

    queue_init(&q);
    for (i = 1; i <= QUEUE_CAPACITY; i++) {
        enqueue(&q, make_vehicle(i));
    }
    CHECK(enqueue(&q, make_vehicle(500)) == 0);
    CHECK(dequeue(&q, &out) == 1);
    CHECK(enqueue(&q, make_vehicle(501)) == 1);     /* freed slot accepts again */
    CHECK(q.dropped == 1);                          /* earlier drop still counted once */
}

static void test_wraparound_keeps_order(void)
{
    VehicleQueue q;
    Vehicle out;
    unsigned int next = 1, expect = 1;
    int round;

    queue_init(&q);
    for (round = 0; round < 3; round++) {           /* keep a few vehicles waiting */
        CHECK(enqueue(&q, make_vehicle(next++)) == 1);
    }
    for (round = 0; round < 5 * QUEUE_CAPACITY; round++) {
        CHECK(enqueue(&q, make_vehicle(next++)) == 1);
        CHECK(dequeue(&q, &out) == 1);
        CHECK(out.id == expect++);                  /* FIFO across 5 wrap-arounds */
    }
    while (dequeue(&q, &out)) {
        CHECK(out.id == expect++);
    }
    CHECK(expect == next);                          /* nothing lost, nothing duplicated */
}

static void test_conservation_in_one_queue(void)
{
    VehicleQueue q;
    Vehicle out;
    unsigned int offered = 0, taken = 0, i;

    queue_init(&q);
    for (i = 0; i < 3 * QUEUE_CAPACITY; i++) {
        offered++;
        enqueue(&q, make_vehicle(i + 1));
        if (i % 3 == 0 && dequeue(&q, &out)) {
            taken++;
        }
    }
    /* every offered vehicle was either taken, still queued, or counted as dropped */
    CHECK(offered == taken + (unsigned int)q.size + q.dropped);
}

static void test_null_arguments_are_safe(void)
{
    VehicleQueue q;
    Vehicle out;

    queue_init(&q);
    queue_init(NULL);
    CHECK(enqueue(NULL, make_vehicle(1)) == 0);
    CHECK(dequeue(NULL, &out) == 0);
    CHECK(queue_is_empty(NULL) == 1);
    CHECK(queue_is_full(NULL) == 1);

    enqueue(&q, make_vehicle(1));
    CHECK(dequeue(&q, NULL) == 0);                  /* NULL output must not crash */
    CHECK(q.size == 1);                             /* and must not consume the vehicle */
}

int main(void)
{
    printf("test_queue_integrity\n");
    TEST_RUN(test_init_clears_state);
    TEST_RUN(test_dequeue_on_empty_is_safe);
    TEST_RUN(test_fifo_order);
    TEST_RUN(test_fill_to_exact_capacity_then_overflow);
    TEST_RUN(test_space_is_reusable_after_overflow);
    TEST_RUN(test_wraparound_keeps_order);
    TEST_RUN(test_conservation_in_one_queue);
    TEST_RUN(test_null_arguments_are_safe);
    return TEST_SUMMARY();
}
