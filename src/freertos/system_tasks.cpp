#include <FreeRTOS.h>

#if (configSUPPORT_STATIC_ALLOCATION == 1)
static StaticTask_t __attribute__((section(".ccmram"))) idle_task;
static StackType_t __attribute__((section(".ccmram"))) idle_stack[configMINIMAL_STACK_SIZE];
extern "C" void vApplicationGetIdleTaskMemory(StaticTask_t **task,
    StackType_t **stack,
    uint32_t *stack_size) {
    *task = &idle_task;
    *stack = idle_stack;
    *stack_size = configMINIMAL_STACK_SIZE;
}
#endif

#if (configSUPPORT_STATIC_ALLOCATION == 1) && (configUSE_TIMERS == 1)
static StaticTask_t __attribute__((section(".ccmram"))) timer_task;
static StackType_t __attribute__((section(".ccmram"))) timer_stack[configTIMER_TASK_STACK_DEPTH];
extern "C" void vApplicationGetTimerTaskMemory(StaticTask_t **task,
    StackType_t **stack,
    uint32_t *stack_size) {
    *task = &timer_task;
    *stack = timer_stack;
    *stack_size = configTIMER_TASK_STACK_DEPTH;
}
#endif
