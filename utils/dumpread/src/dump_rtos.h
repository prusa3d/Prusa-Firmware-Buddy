// dump_rtos.h
#ifndef _DUMP_RTOS_H
#define _DUMP_RTOS_H

#include <crash_dump/dump.h>

#define DUMP_RTOS_MAX_TASKS      16 // actually 8 tasks used
#define DUMP_RTOS_MAX_PRIORITIES 7  // configMAX_PRIORITIES = 7

#define DUMP_RTOS_TASK_STATE_SUSPENDED 0
#define DUMP_RTOS_TASK_STATE_DELAYED   1
#define DUMP_RTOS_TASK_STATE_READY     2
#define DUMP_RTOS_TASK_STATE_RUNNING   3

#pragma pack(push)
#pragma pack(1)

typedef struct _dump_tcb_t {
    uint32_t pxTopOfStack;
    uint32_t xStateListItem[5];
    uint32_t xEventListItem[5];
    uint32_t uxPriority;
    uint32_t pxStack;
    char pcTaskName[16];
    uint32_t uxTCBNumber;
    uint32_t uxTaskNumber;
    uint32_t uxBasePriority;
    uint32_t uxMutexesHeld;
} dump_tcb_t;

typedef struct _dump_listitem_t {
    uint32_t xItemValue;
    uint32_t pxNext;
    uint32_t pxPrevious;
    uint32_t pvOwner;
    uint32_t pvContainer;
} dump_listitem_t;

typedef struct _dump_minlistitem_t {
    uint32_t xItemValue;
    uint32_t pxNext;
    uint32_t pxPrevious;
} dump_minlistitem_t;

typedef struct _dump_list_t {
    uint32_t uxNumberOfItems;
    uint32_t pxIndex;
    dump_minlistitem_t xListEnd;
} dump_list_t;

typedef struct _dump_queue_t {
    uint32_t pcHead;                   /*< Points to the beginning of the queue storage area. */
    uint32_t pcTail;                   /*< Points to the byte at the end of the queue storage area.  Once more byte is allocated than necessary to store the queue items, this is used as a marker. */
    uint32_t pcWriteTo;                /*< Points to the free next place in the storage area. */

    union                              /* Use of a union is an exception to the coding standard to ensure two mutually exclusive structure members don't appear simultaneously (wasting RAM). */
    {
        uint32_t pcReadFrom;           /*< Points to the last place that a queued item was read from when the structure is used as a queue. */
        uint32_t uxRecursiveCallCount; /*< Maintains a count of the number of times a recursive mutex has been recursively 'taken' when the structure is used as a mutex. */
    } u;

    dump_list_t xTasksWaitingToSend;    /*< List of tasks that are blocked waiting to post onto this queue.  Stored in priority order. */
    dump_list_t xTasksWaitingToReceive; /*< List of tasks that are blocked waiting to read from this queue.  Stored in priority order. */

    uint32_t uxMessagesWaiting;         /*< The number of items currently in the queue. */
    uint32_t uxLength;                  /*< The length of the queue defined as the number of items it will hold, not the number of bytes. */
    uint32_t uxItemSize;                /*< The size of each items that the queue will hold. */

    int8_t cRxLock;
    int8_t cTxLock;
    uint32_t uxQueueNumber;
    uint8_t ucQueueType;
} dump_queue_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern uint32_t dump_rtos_task_stacksize(dump_t *pd, uint32_t task_handle);
extern uint32_t dump_rtos_list_task_handles(dump_t *pd, uint32_t list_addr, uint32_t *task_handles);
extern void dump_rtos_print_queue(dump_t *pd, mapfile_t *pm, uint32_t queue_handle);
extern void dump_rtos_print(dump_t *pd, mapfile_t *pm);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_DUMP_RTOS_H
