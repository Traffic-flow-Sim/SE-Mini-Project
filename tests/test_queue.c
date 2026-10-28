
#include <assert.h>
#include <stdio.h>

#include "../queue.h"

static Vehicle make_vehicle(unsigned int id)
{
    Vehicle vehicle = {id, id * 10};
    return vehicle;
}

static void test_empty_queue(void)
{
    VehicleQueue queue;
    Vehicle result;

    queue_init(&queue);

    assert(queue_is_empty(&queue));
    assert(!dequeue(&queue, &result));
    assert(queue.size == 0);
}

static void test_fifo_order(void)
{
    VehicleQueue queue;
    Vehicle result;

    queue_init(&queue);

    assert(enqueue(&queue, make_vehicle(1)));
    assert(enqueue(&queue, make_vehicle(2)));
    assert(enqueue(&queue, make_vehicle(3)));

    assert(dequeue(&queue, &result));
    assert(result.id == 1);

    assert(dequeue(&queue, &result));
    assert(result.id == 2);

    assert(dequeue(&queue, &result));
    assert(result.id == 3);

    assert(queue_is_empty(&queue));
}

static void test_full_queue(void)
{
    VehicleQueue queue;
    Vehicle result;
    unsigned int i;

    queue_init(&queue);

    for (i = 1; i <= QUEUE_CAPACITY; i++) {
        assert(enqueue(&queue, make_vehicle(i)));
    }

    assert(queue_is_full(&queue));
    assert(!enqueue(&queue, make_vehicle(QUEUE_CAPACITY + 1)));
    assert(queue.size == QUEUE_CAPACITY);

    for (i = 1; i <= QUEUE_CAPACITY; i++) {
        assert(dequeue(&queue, &result));
        assert(result.id == i);
    }

    assert(queue_is_empty(&queue));
}

static void test_circular_wraparound(void)
{
    VehicleQueue queue;
    Vehicle result;
    unsigned int i;

    queue_init(&queue);

    for (i = 1; i <= QUEUE_CAPACITY; i++) {
        assert(enqueue(&queue, make_vehicle(i)));
    }

    /* Free four slots at the front. */
    for (i = 1; i <= 4; i++) {
        assert(dequeue(&queue, &result));
        assert(result.id == i);
    }

    /* These insertions wrap around to the start of the array. */
    for (i = 9; i <= 12; i++) {
        assert(enqueue(&queue, make_vehicle(i)));
    }

    /* FIFO order must survive wraparound. */
    for (i = 5; i <= 12; i++) {
        assert(dequeue(&queue, &result));
        assert(result.id == i);
    }

    assert(queue_is_empty(&queue));
}

int main(void)
{
    test_empty_queue();
    test_fifo_order();
    test_full_queue();
    test_circular_wraparound();

    printf("All queue tests passed.\n");
    return 0;
}
