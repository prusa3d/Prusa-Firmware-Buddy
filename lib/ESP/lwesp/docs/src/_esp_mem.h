/**
 * \addtogroup      ESP_MEM
 * \{
 *
 * Memory manager is light-weight implementation of malloc and free functions in standard C language.
 *
 * It uses `FIRST-FIT` allocation algorithm meaning it uses the first free region which suits available length of memory.
 *
 * On top of everything, it supports different memory regions which means user doesn't need to provide one full block of memory.
 *
 * \par             Memory regions
 *
 * Memory regions are a way how to provide splitted memory organizations.
 *
 * Consider use of external RAM memory (usually SDRAM), where user would like to use it as dynamic memory (heap) 
 * together with internal RAM on target device.
 *
 * By defining start address and length of regions, user can use unlimited number of regions.
 *
 * \note            When assigning regions, next region must always have greater memory address than one before.
 *
 * \note            The only limitation of regions is that region must be addressible in memory space.
 *                  If external memory is used, it must have memory mapping hardware to take care of addressing.
 *
 * Examples of defining `2` regions, one in internal memory, second on external SDRAM memory
 *
 * \include         _example_mem.c
 *
 * \note            Even with multiple regions, maximal allocation size is length of biggest region.
 *                  In case of example, we have `0x9000` bytes of memory but theoretically maximal block may have `0x8000` bytes. 
 *                  Practically maximal value is little lower due to header values required to track blocks.
 *
 * \par             Allocating memory
 *
 * Now once we have set regions, we can proceed with allocating the memory.
 * On beginning, all regions are big blocks of free memory (number of big blocks equals number of regions as all regions are free)
 * and these blocks will be smaller, depends on allocating and freeing.
 *
 * Image below shows how memory structure looks like after some time of allocating and freeing memory regions.
 * <b>WHITE</b> blocks are free regions (ready to be allocated) and <b>RED</b> blocks are already allocated and used by user.
 *
 * When block is allocated, it is removed from list of free blocks so that we only track list of free blocks.
 * This is visible with connections of white blocks together.
 *
 * \image html memory_manager_structure.svg Memory structure after allocating and freeing.
 *
 * \par             Freeing memory
 * 
 * When we try to free used memory, it is set as free and then inserted between `2` free blocks
 * which is visible on first part of image below.
 *
 * Later, we also decide to free block which is just between `2` free blocks.
 * When we do it, manager will detect that there is free memory before and after used one
 * and will mark everything together into one big block, which is visible on second part of image below.
 *
 * \image html memory_manager_structure_freeing.svg Memory structure after freeing `2` blocks
 *
 * \par             Custom allocator
 *
 * it is possible to have custom allocator for memory, specially useful when working on large firmware with
 * many resources asking for dynamic memory manager.
 *
 * To enable custom allocator, \ref ESP_CFG_MEM_CUSTOM must be set to `1`.
 * Following this change, it is necessary to implement these functions:
 *
 *  - \ref esp_mem_malloc
 *  - \ref esp_mem_calloc
 *  - \ref esp_mem_realloc
 *  - \ref esp_mem_free
 *
 * \note            When using custom allocator, user must take care of multiple threads accessing to the same resource
 * 
 * \}
 */