#include "topics.hpp"
#include "FreeRTOS.h"
#include "task.h"

namespace Examples {
struct Sample { float value; };
static TypedTopicPublisher<Sample> publisher("example/sample");
static TypedTopicSubscriber<Sample> subscriber("example/sample", 4U);
// File-scope registration must happen before App_Init(). This example uses task
// critical sections on both sides because the overwrite ring shares its head.
bool ExchangeSample(float value, Sample& out)
{
    taskENTER_CRITICAL();
    const bool sent = publisher.Publish(Sample{value});
    const bool received = subscriber.TryGet(&out);
    taskEXIT_CRITICAL();
    return sent && received;
}
}
