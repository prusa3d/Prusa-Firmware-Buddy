// dump_rtos.c

#include "dump_rtos.h"
#include <stdlib.h>
#include <string.h>

const char *dump_rtos_task_state_text[4] = {
    "SUSPENDED",
    "DELAYED",
    "READY",
    "RUNNING",
};

uint32_t dump_rtos_task_stacksize(dump_t *pd, uint32_t task_handle) {
    dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
    if (strcmp(ptcb->pcTaskName, "defaultTask") == 0)
        return 1024 * 4;
    else if (strcmp(ptcb->pcTaskName, "displayTask") == 0)
        return 2048 * 4;
    else if (strcmp(ptcb->pcTaskName, "webServerTask") == 0)
        return 1024 * 4;
    else if (strcmp(ptcb->pcTaskName, "measurementTask") == 0)
        return 512 * 4;
    else if (strcmp(ptcb->pcTaskName, "IDLE") == 0)
        return 128 * 4;
    else if (strcmp(ptcb->pcTaskName, "USBH_Thread") == 0)
        return 128 * 4;
    else if (strcmp(ptcb->pcTaskName, "tcpip_thread") == 0)
        return 1024 * 4;
    else if (strcmp(ptcb->pcTaskName, "EthIf") == 0)
        return 350 * 4;
    return 0;
}

uint32_t dump_rtos_list_task_handles(dump_t *pd, uint32_t list_addr, uint32_t *task_handles) {
    dump_list_t *plist = (dump_list_t *)dump_get_data_ptr(pd, list_addr);
    uint32_t index = 0;
    dump_listitem_t *plistitem = (dump_listitem_t *)dump_get_data_ptr(pd, plist->pxIndex);
    if (plistitem->pvOwner == 0)
        plistitem = (dump_listitem_t *)dump_get_data_ptr(pd, plistitem->pxNext);
    while (index < plist->uxNumberOfItems) {
        if (plistitem->pvOwner != 0) {
            task_handles[index] = plistitem->pvOwner;
            index++;
        }
        plistitem = (dump_listitem_t *)dump_get_data_ptr(pd, plistitem->pxNext);
    }
    return index;
}

void dump_rtos_print_queue(dump_t *pd, mapfile_t *pm, uint32_t queue_handle) {
    dump_queue_t *pqueue = (dump_queue_t *)dump_get_data_ptr(pd, queue_handle);
    uint32_t data_addr = queue_handle + sizeof(dump_queue_t) + 5;
    void *pqueue_data = dump_get_data_ptr(pd, data_addr);
    printf(" .pcHead = 0x%08x (+%d)\n", pqueue->pcHead, pqueue->pcHead - data_addr);
    printf(" .pcTail = 0x%08x (+%d)\n", pqueue->pcTail, pqueue->pcTail - data_addr);
    printf(" .pcWriteTo = 0x%08x (+%d)\n", pqueue->pcWriteTo, pqueue->pcWriteTo - data_addr);
    printf(" .pcReadFrom = 0x%08x (+%d)\n", pqueue->u.pcReadFrom, pqueue->u.pcReadFrom - data_addr);
    //	dump_list_t xTasksWaitingToSend;
    //	dump_list_t xTasksWaitingToReceive;
    printf(" .uxMessagesWaiting = 0x%08x\n", pqueue->uxMessagesWaiting);
    printf(" .uxLength = 0x%08x\n", pqueue->uxLength);
    printf(" .uxItemSize = 0x%08x\n", pqueue->uxItemSize);
    //	int8_t cRxLock;
    //	int8_t cTxLock;
    printf(" .uxQueueNumber = 0x%08x\n", pqueue->uxQueueNumber);
    printf(" .ucQueueType = 0x%02x\n", pqueue->ucQueueType);
    if (pqueue->uxItemSize == 1) {
        uint8_t *pd = (uint8_t *)pqueue_data;
        for (int i = 0; i < pqueue->uxLength; i++)
            printf(" +%04x %02x '%c' (%d)\n", i, pd[i], pd[i], pd[i]);
    }
}

void dump_rtos_print(dump_t *pd, mapfile_t *pm) {
    if (!pm)
        return;
    printf("\nRTOS\n");
    mapfile_mem_entry_t *e;
    uint32_t uxCurrentNumberOfTasks = 0;
    uint32_t xIdleTaskHandle = 0;
    uint32_t defaultTaskHandle = 0;
    uint32_t displayTaskHandle = 0;
    uint32_t webServerTaskHandle = 0;
    uint32_t pxCurrentTCB = 0;
    dump_print_var_ui32(pd, pm, "xFreeBytesRemaining");
    dump_print_var_ui32(pd, pm, "xMinimumEverFreeBytesRemaining");
    if ((e = dump_print_var_ui32(pd, pm, "uxCurrentNumberOfTasks")) != NULL)
        uxCurrentNumberOfTasks = dump_get_ui32(pd, e->addr);
    dump_print_var_ui32(pd, pm, "xTickCount");
    dump_print_var_ui32(pd, pm, "uxTopReadyPriority");
    dump_print_var_ui32(pd, pm, "xSchedulerRunning");
    dump_print_var_ui32(pd, pm, "uxPendedTicks");
    dump_print_var_ui32(pd, pm, "xYieldPending");
    dump_print_var_ui32(pd, pm, "xNumOfOverflows");
    dump_print_var_ui32(pd, pm, "uxTaskNumber");
    dump_print_var_ui32(pd, pm, "xNextTaskUnblockTime");
    if ((e = dump_print_var_ui32(pd, pm, "xIdleTaskHandle")) != NULL)
        xIdleTaskHandle = dump_get_ui32(pd, e->addr);

    if ((e = dump_print_var_ui32(pd, pm, "defaultTaskHandle")) != NULL)
        defaultTaskHandle = dump_get_ui32(pd, e->addr);
    if ((e = dump_print_var_ui32(pd, pm, "displayTaskHandle")) != NULL)
        displayTaskHandle = dump_get_ui32(pd, e->addr);
    if ((e = dump_print_var_ui32(pd, pm, "webServerTaskHandle")) != NULL)
        webServerTaskHandle = dump_get_ui32(pd, e->addr);

    if ((e = dump_print_var_ui32(pd, pm, "pxCurrentTCB")) != NULL)
        pxCurrentTCB = dump_get_ui32(pd, e->addr);

    // all currently created tasks sorted by tasknumber
    uint32_t task_handles[DUMP_RTOS_MAX_TASKS];
    memset(task_handles, 0, sizeof(task_handles));
    uint32_t task_states[DUMP_RTOS_MAX_TASKS];
    uint32_t task_count = 0;

    // we need enumerate all task lists in RTOS to retreive list of all created tasks
    uint32_t task_handles_ready[DUMP_RTOS_MAX_PRIORITIES][DUMP_RTOS_MAX_TASKS];
    uint32_t task_handles_ready_count[DUMP_RTOS_MAX_PRIORITIES] = { 0, 0, 0, 0, 0, 0, 0 };
    if ((e = dump_print_var(pd, pm, "pxReadyTasksLists")) != NULL) {
        for (int pri = 0; pri < DUMP_RTOS_MAX_PRIORITIES; pri++) {
            printf("priority %d\n", pri);
            fflush(stdout);
            task_handles_ready_count[pri] = dump_rtos_list_task_handles(pd, e->addr + pri * sizeof(dump_list_t), task_handles_ready[pri]);
            for (int task = 0; task < task_handles_ready_count[pri]; task++) {
                uint32_t task_handle = task_handles_ready[pri][task];
                dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
                if (task_handles[ptcb->uxTCBNumber - 1] == 0)
                    task_count++;
                task_handles[ptcb->uxTCBNumber - 1] = task_handle;
                if (task_handle == pxCurrentTCB)
                    task_states[ptcb->uxTCBNumber - 1] = DUMP_RTOS_TASK_STATE_RUNNING;
                else
                    task_states[ptcb->uxTCBNumber - 1] = DUMP_RTOS_TASK_STATE_READY;
                printf(" handle 0x%08x number %u name %s\n", task_handle, ptcb->uxTCBNumber, ptcb->pcTaskName);
                fflush(stdout);
            }
        }
    }

    uint32_t task_handles_delayed[DUMP_RTOS_MAX_TASKS];
    uint32_t task_handles_delayed_count = 0;
    if ((e = dump_print_var(pd, pm, "pxDelayedTaskList")) != NULL) {
        task_handles_delayed_count = dump_rtos_list_task_handles(pd, dump_get_ui32(pd, e->addr), task_handles_delayed);
        for (int task = 0; task < task_handles_delayed_count; task++) {
            uint32_t task_handle = task_handles_delayed[task];
            dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
            if (task_handles[ptcb->uxTCBNumber - 1] == 0)
                task_count++;
            task_handles[ptcb->uxTCBNumber - 1] = task_handle;
            task_states[ptcb->uxTCBNumber - 1] = DUMP_RTOS_TASK_STATE_DELAYED;
            printf(" handle=0x%08x number=%u name=%s\n", task_handle, ptcb->uxTCBNumber, ptcb->pcTaskName);
        }
    }

    uint32_t task_handles_overflow[DUMP_RTOS_MAX_TASKS];
    uint32_t task_handles_overflow_count = 0;
    if ((e = dump_print_var(pd, pm, "pxOverflowDelayedTaskList")) != NULL) {
        task_handles_overflow_count = dump_rtos_list_task_handles(pd, dump_get_ui32(pd, e->addr), task_handles_overflow);
        for (int task = 0; task < task_handles_overflow_count; task++) {
            uint32_t task_handle = task_handles_overflow[task];
            dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
            if (task_handles[ptcb->uxTCBNumber - 1] == 0)
                task_count++;
            task_handles[ptcb->uxTCBNumber - 1] = task_handle;
            task_states[ptcb->uxTCBNumber - 1] = DUMP_RTOS_TASK_STATE_DELAYED;
            printf(" handle=0x%08x number=%u name=%s\n", task_handle, ptcb->uxTCBNumber, ptcb->pcTaskName);
        }
    }

    uint32_t task_handles_suspended[DUMP_RTOS_MAX_TASKS];
    uint32_t task_handles_suspended_count = 0;
    if ((e = dump_print_var(pd, pm, "xSuspendedTaskList")) != NULL) {
        task_handles_suspended_count = dump_rtos_list_task_handles(pd, e->addr, task_handles_suspended);
        for (int task = 0; task < task_handles_suspended_count; task++) {
            uint32_t task_handle = task_handles_suspended[task];
            dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
            if (task_handles[ptcb->uxTCBNumber - 1] == 0)
                task_count++;
            task_handles[ptcb->uxTCBNumber - 1] = task_handle;
            task_states[ptcb->uxTCBNumber - 1] = DUMP_RTOS_TASK_STATE_SUSPENDED;
            printf(" handle=0x%08x number=%u name=%s\n", task_handle, ptcb->uxTCBNumber, ptcb->pcTaskName);
        }
    }

    printf("RTOS TASKs (%u)\n", task_count);
    printf(" num handle     state     pri stack      stacktop   name\n");
    for (int task = 0; task < task_count; task++) {
        uint32_t task_handle = task_handles[task];
        uint32_t stacksize = dump_rtos_task_stacksize(pd, task_handle);
        uint32_t task_state = task_states[task];
        dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
        printf(" %-3u 0x%08x %-9s %u/%u 0x%08x 0x%08x %s\n", ptcb->uxTCBNumber, task_handle, dump_rtos_task_state_text[task_state], ptcb->uxBasePriority, ptcb->uxPriority, ptcb->pxStack, ptcb->pxTopOfStack, ptcb->pcTaskName);
        char sym[32] = "";
        sprintf(sym, "ucHeap.TCB_%s", ptcb->pcTaskName);
        dump_add_symbol(task_handle, sizeof(dump_tcb_t), sym);
        sprintf(sym, "ucHeap.STACK_%s", ptcb->pcTaskName);
        dump_add_symbol(ptcb->pxStack, stacksize, sym);
    }
    printf("RTOS TASK STACKs\n");
    printf(" num stack      stacktop   size   depth maxdepth name\n");
    for (int task = 0; task < task_count; task++) {
        uint32_t task_handle = task_handles[task];
        uint32_t stacksize = dump_rtos_task_stacksize(pd, task_handle);
        dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
        uint32_t stackmax = ptcb->pxStack + stacksize;
        float depth = 100.0F * (stackmax - ptcb->pxTopOfStack) / stacksize;
        uint8_t *pstack = dump_get_data_ptr(pd, ptcb->pxStack);
        uint32_t o = 0;
        for (; o < stackmax; o++)
            if (pstack[o] != 0xa5)
                break;
        float maxdepth = 100.0F * (stacksize - o) / stacksize;
        printf(" %-3u 0x%08x 0x%08x 0x%04x %5.1f%% %5.1f%% %s\n", ptcb->uxTCBNumber, ptcb->pxStack, ptcb->pxTopOfStack, stacksize, depth, maxdepth, ptcb->pcTaskName);
    }
    if (1)
        for (int task = 0; task < task_count; task++) {
            uint32_t task_handle = task_handles[task];
            uint32_t stacksize = dump_rtos_task_stacksize(pd, task_handle);
            dump_tcb_t *ptcb = (dump_tcb_t *)dump_get_data_ptr(pd, task_handle);
            uint32_t stacktop = ptcb->pxTopOfStack;
            uint32_t stackmax = ptcb->pxStack + stacksize;
            printf("\nTASK STACK (%s)\n", ptcb->pcTaskName);
            dump_print_stack(pd, pm, stacktop, (stackmax - stacktop) / 4);
            //    if (xIdleTaskHandle)
            //    	dump_rtos_task_stacksize(pd, xIdleTaskHandle);
        }
}
