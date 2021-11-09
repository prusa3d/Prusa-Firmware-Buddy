/* External memory description */
#define EXT_MEM_START_ADDR      0xD0000000 /* Memory start address */
#define EXT_MEM_SIZE            0x00800000 /* Total size of external memory */
#define EXT_MEM_USED_SIZE       0x00008000 /* Size of memory application needs for stack */

/* EXT_MEM_USED_SIZE must be smaller than EXT_MEM_SIZE */

uint8_t initialized = 0;

/*
 * \brief           Core callback function which must be implemented by user
 * \param[in]       ll: Low-Level structure
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_ll_init(esp_ll_t* ll) {
#if !ESP_CFG_MEM_CUSTOM
    /* Make sure that external memory is addressable in the memory area at this point */
    /* Or initialize it now */

    /* Use memory block for stack at the end of external memory. Its size is EXT_MEM_USED_SIZE bytes */
    /* !!! Make sure that other part of application never writes to this area !!! */
    uint8_t* memory = (void *)(EXT_MEM_START_ADDR + EXT_MEM_SIZE - EXT_MEM_USED_SIZE);

    /* Define memory regions */
    esp_mem_region_t mem_regions[] = {
        { memory /* Pointer to memory, now points to external memory */, EXT_MEM_USED_SIZE /* Size of memory in bytes */ }
    };
    if (!initialized) {
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions)); 
    }
#endif /* !ESP_CFG_MEM_CUSTOM */

    /* Do other steps */
}