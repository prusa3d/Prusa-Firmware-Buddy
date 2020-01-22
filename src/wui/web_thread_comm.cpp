// web_thread_comm.cpp
//  Test source file for inter thread communication with GUI thread.

#include "web_thread_comm.h"
#include "string.h"

web_thread_communication_t web_thread_comm;
shared_mem_t * web_shared_memory = nullptr;
osThreadId web_task = (void*)0; // task handle
osMessageQId web_queue = (void*)0; // input queue (uint32_t)
osSemaphoreId web_sema = (void*)0; // semaphore handle
osPoolId shared_mem_ID;
uint8_t web_thread_comm_init(){
/*
    memset(&web_thread_comm, 0, sizeof(web_thread_comm));
    osMessageQDef(webQueue, 64, uint32_t);
    web_queue = osMessageCreate(osMessageQ(webQueue), NULL);
    osSemaphoreDef(webSema);
    web_sema = osSemaphoreCreate(osSemaphore(webSema), 1);
    web_task = osThreadGetId();
*/
    osPoolDef(MemPool, 1, shared_mem_t);
    shared_mem_ID = osPoolCreate(osPool(MemPool));
    if(shared_mem_ID != 0){
        web_shared_memory = (shared_mem_t*) osPoolAlloc(shared_mem_ID);
        if(web_shared_memory != NULL)
            return 0;
    }
    return 1;

}

void web_thread_comm_loop(){

    osEvent ose;
    web_thread_comm.flags = 0;
    while(1){
        while ((ose = osMessageGet(web_queue, 0)).status == osEventMessage){
            web_thread_comm.value = ose.value.v;
            web_thread_comm.flags |= 1;
        }
        if(web_thread_comm.flags & 1){
            if(web_thread_comm.value == 23){
                web_thread_comm.flags = 2;
            }
            web_thread_comm.flags &= ~1;
        }
    }



}
