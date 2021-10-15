/*
 * \brief           Create a new mutex and pass it to input pointer
 * \note            This function is required with OS
 * \note            Recursive mutex must be created as it may be locked multiple times before unlocked
 * \param[out]      p: Pointer to mutex structure to save result to
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_create(esp_sys_mutex_t* p) {
    osMutexDef(MUT);                            /* Define a mutex */
    *p = osRecursiveMutexCreate(osMutex(MUT));  /* Create recursive mutex */
    return *p != NULL;
}

/*
 * \brief           Delete mutex from OS
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_delete(esp_sys_mutex_t* p) {
    return osMutexDelete(*p) == osOK;           /* Delete mutex */
}

/*
 * \brief           Wait forever to lock the mutex
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_lock(esp_sys_mutex_t* p) {
    return osRecursiveMutexWait(*p, osWaitForever) == osOK; /* Wait forever for mutex */
}

/*
 * \brief           Unlock mutex
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_unlock(esp_sys_mutex_t* p) {
    return osRecursiveMutexRelease(*p) == osOK; /* Release mutex */
}

/*
 * \brief           Check if mutex structure is valid OS entry
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_isvalid(esp_sys_mutex_t* p) {
    return *p != NULL;                          /* Check if mutex is valid */
}

/*
 * \brief           Set mutex structure as invalid
 * \note            This function is required with OS
 * \param[in]       p: Pointer to mutex structure
 * \return          1 on success, 0 otherwise
 */
uint8_t
esp_sys_mutex_invalid(esp_sys_mutex_t* p) {
    *p = ESP_SYS_MUTEX_NULL;                    /* Set mutex as invalid */
    return 1;
}