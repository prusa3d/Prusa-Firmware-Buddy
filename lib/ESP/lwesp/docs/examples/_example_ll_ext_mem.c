uint8_t initialized = 0;

/*
 * \brief           Core callback function which must be implemented by user
 * \param[in]       ll: Low-Level structure
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_ll_init(esp_ll_t* ll) {
#if !ESP_CFG_MEM_CUSTOM
    /* Make sure that external memory is addressable in the memory area */

    /* Read documentation of your device where external memory is available in address space */
    uint8_t* memory = (void *)0x12345678;
    esp_mem_region_t mem_regions[] = {
        { memory /* Pointer to memory */, 0x1234 /* Size of memory in bytes */ }
    };
    if (!initialized) {
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions)); 
    }
#endif /* !ESP_CFG_MEM_CUSTOM */

    /* Do other steps */
}