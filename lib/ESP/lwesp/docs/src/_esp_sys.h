/**
 * \addtogroup      ESP_SYS
 * \{
 *
 * System functions are required for timing functions and
 * for implementation of OS specific functions.
 *
 * \section         sect_sys_req Required implementation
 *
 * Implementation required functions depend on 
 * usage of OS system or not.
 *
 * System init function \ref esp_sys_init is called on startup,
 * where user can set default states or initialize protection mutex
 *
 * \par             Always required functions
 *
 * Timing function is always required.
 * it must return time in units of milliseconds.
 *
 * For STM32 users with HAL driver support,
 * this function can be implemented by returning `osKernelSystick` function, implemented by `CMSIS-OS` ready RTOS.
 *
 * \par             OS required functions
 *
 * When OS is involved, all other functions must be created by user.
 * When using provided template, CMSIS-OS implementation is already prepared.
 * Some of CMSIS-OS supported operating systems:
 *
 *  - Keil RTX
 *  - FreeRTOS is not by default, but `cmsis_os.c` wrapper exists
 *      which allows you compatibility with standard CMSIS-OS functions
 * 
 * \section         sect_os_functions OS functions
 *
 * \par             Protection functions
 *
 * Core protection functions must be implemented by user.
 * They are called when we have to protect core from multiple access.
 *
 * \note            Keep in mind that these functions may be called recursively.
 *                  If you do protection using mutex, use recursive mutex support!
 *
 *  - \ref esp_sys_protect function increments protection counter
 *  - \ref esp_sys_unprotect function decrements protection counter.
 *          When set to 0, core is unprotected again
 *
 * \include         _example_sys_core.c
 *
 * \par             Mutexes
 *
 * Some functions below must be implemented for mutex management.
 *
 * Please read function documentation what is the purpose of functions
 *
 *  - \ref esp_sys_mutex_create
 *  - \ref esp_sys_mutex_delete
 *  - \ref esp_sys_mutex_lock
 *  - \ref esp_sys_mutex_unlock
 *  - \ref esp_sys_mutex_isvalid
 *  - \ref esp_sys_mutex_invalid
 *
 * \include         _example_sys_mutex.c
 *
 * \par             Semaphores
 *
 * For thread synchronization, binary semaphores are used.
 * They must only use single token and may be free or taken.
 *
 *  - \ref esp_sys_sem_create(esp_sys_sem_t* p, uint8_t cnt);
 *  - \ref esp_sys_sem_delete(esp_sys_sem_t* p);
 *  - \ref esp_sys_sem_wait(esp_sys_sem_t* p, uint32_t timeout);
 *  - \ref esp_sys_sem_release(esp_sys_sem_t* p);
 *  - \ref esp_sys_sem_isvalid(esp_sys_sem_t* p);
 *  - \ref esp_sys_sem_invalid(esp_sys_sem_t* p);
 *
 * \include         _example_sys_semaphore.c
 *
 * \par             Message queues
 *
 * Message queues are used for thread communication in safe way.
 *
 *  - \ref esp_sys_mbox_create
 *  - \ref esp_sys_mbox_delete
 *  - \ref esp_sys_mbox_put
 *  - \ref esp_sys_mbox_get
 *  - \ref esp_sys_mbox_putnow
 *  - \ref esp_sys_mbox_getnow
 *  - \ref esp_sys_mbox_isvalid
 *  - \ref esp_sys_mbox_invalid
 *
 * \include         _example_sys_mbox.c
 *
 * \par             Threads
 *
 * To prevent wrongly setup of core threads,
 * threading management functions must be implemented and used on demand from ESP stack.
 *
 *  - \ref esp_sys_thread_create is called when a new thread should be created
 *  - \ref esp_sys_thread_terminate is called to terminate thread
 *  - \ref esp_sys_thread_yield to yield current thread and allow processing other threads
 *
 * \include         _example_sys_thread.c
 *
 * \}
 */