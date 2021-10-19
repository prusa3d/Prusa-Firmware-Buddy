/**
 * \addtogroup      ESP_PBUF
 * \{
 *
 * Packet buffer is special memory for incoming (received) network data on active connections.
 *
 * Purpose of it is to have a support of making one big buffer from chunks of fragmented received data,
 * without need of having one big linear array.
 *
 * \image html pbuf_block_diagram.svg Block diagram of pbuf chain
 *
 * From the image above, we can see that we can chain buffers together to create quasi-linear block of data.
 * Each packet buffer consists of:
 *
 *  - Pointer to next pbuf in a chain or `NULL` if last one
 *  - Length of current pbuf
 *  - Length of current and all next in chain
 *      - When pbuf is last, this value is the same as length of it
 *  - Reference counter, which holds number of pointers pointing to this block.
 *
 * If we describe image more into details, there are `3` pbufs in chain and we can see that
 * there is some variable involved with pointing to second pbuf, therefore it has reference count set to `2`.
 *
 * In table below you can see what is written in memory on the image above.
 *
 * <table>
 *  <tr><th>Block number    <th>Next packet buffer  <th>Block size      <th>Total size of chain <th>Reference counter   </tr>
 *  <tr><td>Block 1         <td>Block 2             <td>150             <td>550                 <td>1                   </tr>
 *  <tr><td>Block 2         <td>Block 3             <td>130             <td>400                 <td>2                   </tr>
 *  <tr><td>Block 3         <td>NULL                <td>270             <td>270                 <td>1                   </tr>
 * </table>
 *
 * \par             Reference counter
 *
 * Reference counter is important variable as it holds number of references pointing to block.
 * It is used when user wants to free the block. Since block may be referenced from different locations,
 * doing free from one reference would make undefined behavior for all other references pointing to this pbuf
 *
 * If we go back to image above, we can see that variable points to first pbuf and reference count is set to `1`,
 * which means we are the only one pointing to this pbuf at the moment.
 * If we try to free the memory, this operation is perfectly valid as nobody else is pointing to memory.
 *
 * Steps to remove packet buffers are:
 *
 *  - Decrease reference counter by `1`
 *  - If reference counter is now `0`, free the packet buffer
 *      - Set next pbuf in chain as current and start over
 *  - If reference counter is still not `0`, return from function
 *
 * A new memory structure is visible on image below.
 *
 * \image html pbuf_block_diagram_after_free.svg Block diagram of pbuf chain after free from user variable `1`.
 *
 * <table>
 *  <tr><th>Block number    <th>Next packet buffer  <th>Block size      <th>Total size of chain <th>Reference counter   </tr>
 *  <tr><td>Block 2         <td>Block 3             <td>130             <td>400                 <td>1                   </tr>
 *  <tr><td>Block 3         <td>NULL                <td>270             <td>270                 <td>1                   </tr>
 * </table>
 *
 * \section         sect_pbuf_concat_chain Concatenating vs chaining
 *
 * When we are dealing with application part, it is important to know what is the difference between \ref esp_pbuf_cat and \ref esp_pbuf_chain.
 *
 * Imagine we have `2` pbufs and each of them is pointed to by `2` different variables, like on image below.
 *
 * \image html pbuf_cat_vs_chain_1.svg <b>2</b> independent pbufs with <b>2</b> variables
 *
 * After we call \ref esp_pbuf_cat, we get a new structure which is on image below.
 *
 * \image html pbuf_cat_vs_chain_2.svg Pbufs structure after calling esp_pbuf_cat
 * 
 * We can see that reference of second is still set to `1`, but `2` variables are pointing to this block.
 * After we call \ref esp_pbuf_cat, we have to forget using user variable `2`, because if we somehow try to free pbuf from variable `1`, 
 * then variable `2` points to undefined memory.
 *
 * \include         _example_pbuf_cat.c
 *
 * If we need to link pbuf to another pbuf and we still need to use variable, then use \ref esp_pbuf_chain instead.
 *
 * \image html pbuf_cat_vs_chain_3.svg Pbufs structure after calling esp_pbuf_chain
 *
 * When we use this method, second pbuf has reference set to `2` 
 * and now it is perfectly valid to use our user variable `2` to access the memory.
 *
 * Once we are done using pbuf, we have to free it using \ref esp_pbuf_free function.
 *
 * \include         _example_pbuf_chain.c
 *
 * \section         sect_pbuf_extract Extract data from concatenated pbufs
 *
 * Chain of pbufs does not include linear data memory. To extract data from chained pbufs, we have to read pbuf by pbuf.
 *
 * \include         _example_pbuf_extract.c
 * 
 * \}
 */