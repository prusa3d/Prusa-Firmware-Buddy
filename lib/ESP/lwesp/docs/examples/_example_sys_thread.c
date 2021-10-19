/*
 * \brief           Create a new thread
 * \note            This function is required with OS
 * \param[out]      t: Pointer to thread identifier if create was successful
 * \param[in]       name: Name of a new thread
 * \param[in]       thread_func: Thread function to use as thread body
 * \param[in]       arg: Thread function argument
 * \param[in]       stack_size: Size of thread stack in uints of bytes
 * \param[in]       prio: Thread priority 
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_thread_create(esp_sys_thread_t* t, const char* name, void (*thread_func)(void *), void* const arg, size_t stack_size, esp_sys_thread_prio_t prio) {
    const osThreadDef_t thread_def = {(char *)name, (os_pthread)thread_func, (osPriority)prio, 0, stack_size};  /* Create thread description */
    *t = osThreadCreate(&thread_def, arg);      /* Create thread */
    return *t != NULL;
}

/*
 * \brief           Terminate thread (shut it down and remove)
 * \note            This function is required with OS
 * \param[in]       t: Thread handle to terminate. If set to NULL, terminate current thread (thread from where function is called)
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_thread_terminate(esp_sys_thread_t* t) {
    osThreadTerminate(*t);                      /* Terminate thread */
    return 1;
}

/*
 * \brief           Yield current thread
 * \note            This function is required with OS
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_thread_yield(void) {
    osThreadYield();                            /* Yield current thread */
    return 1;
}
