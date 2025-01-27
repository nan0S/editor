#ifndef PLATFORM_COMMON_H
#define PLATFORM_COMMON_H

struct platform_shared_work_queues
{
 work_queue LowPriorityQueue;
 work_queue HighPriorityQueue;
};

#endif //PLATFORM_COMMON_H
