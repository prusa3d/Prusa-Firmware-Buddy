/*
 * This part should be done in ll initialization function only once on startup
 * Check ESP_LL part of library for more info
 */

#if !ESP_CFG_MEM_CUSTOM

/* We can simply create a big array variable which will be linked to internal memory by linker */
uint8_t mem_int[0x1000];

/*
 * Define memory regions for allocating algorithm,
 * make sure regions are in correct order for memory location
 */
esp_mem_region_t mem_regions[] = {
    { mem_int, sizeof(mem_int) },               /* Set first memory region to internal memory of length 0x1000 bytes */ 
    { (void *)0xC0000000, 0x8000 },             /* External heap memory is located on 0xC0000000 and has 0x8000 bytes of memory */ 
};

/* On startup, user must call function to assign memory regions */
esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions));

#endif /* !ESP_CFG_MEM_CUSTOM */