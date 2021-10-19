/*
 * \brief           Create a new binary semaphore and set initial state
 * \note            Semaphore may only have 1 token available
 * \note            This function is required with OS
 * \param[out]      p: Pointer to semaphore structure to fill with result
 * \param[in]       cnt: Count indicating default semaphore state:
 *                     0: Lock it immediteally
 *                     1: Leave it unlocked
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_sem_create(esp_sys_sem_t* p, uint8_t cnt) {
    osSemaphoreDef(SEM);                        /* Define semaphore info */
    *p = osSemaphoreCreate(osSemaphore(SEM), 1);/* Create semaphore with one token */
    
    if (*p && !cnt) {                           /* We have valid entry */
        osSemaphoreWait(*p, 0);                 /* Lock semaphore immediatelly */
    }
    return *p != NULL;
}

/*
 * \brief           Delete binary semaphore
 * \note            This function is required with OS
 * \param[in]       p: Pointer to semaphore structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_sem_delete(esp_sys_sem_t* p) {
    return osSemaphoreDelete(*p) == osOK;       /* Delete semaphore */
}

/*
 * \brief           Wait for semaphore to be available
 * \note            This function is required with OS
 * \param[in]       p: Pointer to semaphore structure
 * \param[in]       timeout: Timeout to wait in milliseconds. When 0 is applied, wait forever
 * \return          Number of milliseconds waited for semaphore to become available
 */
uint32_t
esp_sys_sem_wait(esp_sys_sem_t* p, uint32_t timeout) {
    uint32_t tick = osKernelSysTick();          /* Get start tick time */
    return (osSemaphoreWait(*p, !timeout ? osWaitForever : timeout) == osOK) ? (osKernelSysTick() - tick) : ESP_SYS_TIMEOUT;    /* Wait for semaphore with specific time */
}

/*
 * \brief           Release semaphore
 * \note            This function is required with OS
 * \param[in]       p: Pointer to semaphore structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_sem_release(esp_sys_sem_t* p) {
    return osSemaphoreRelease(*p) == osOK;      /* Release semaphore */
}

/*
 * \brief           Check if semaphore is valid
 * \note            This function is required with OS
 * \param[in]       p: Pointer to semaphore structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_sem_isvalid(esp_sys_sem_t* p) {
    return *p != NULL;                          /* Check if valid */
}

/*
 * \brief           Invalid semaphore
 * \note            This function is required with OS
 * \param[in]       p: Pointer to semaphore structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_sem_invalid(esp_sys_sem_t* p) {
    *p = ESP_SYS_SEM_NULL;                      /* Invaldiate semaphore */
    return 1;
}