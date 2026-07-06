#include "LoadResultQueue.h"

#include <cassert>
#include <optional>

struct QueueItem {
    int value = 0;
};

static void emptyQueueReturnsNoValue()
{
    LoadResultQueue<QueueItem> queue;
    assert(!queue.tryPop().has_value());
}

static void queueReturnsItemsInFifoOrder()
{
    LoadResultQueue<QueueItem> queue;
    queue.push(QueueItem{1});
    queue.push(QueueItem{2});

    auto first = queue.tryPop();
    auto second = queue.tryPop();
    auto third = queue.tryPop();

    assert(first.has_value());
    assert(second.has_value());
    assert(first->value == 1);
    assert(second->value == 2);
    assert(!third.has_value());
}

int main()
{
    emptyQueueReturnsNoValue();
    queueReturnsItemsInFifoOrder();
    return 0;
}
