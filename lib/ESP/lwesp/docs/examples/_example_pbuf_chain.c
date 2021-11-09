esp_pbuf_p a, b;
                                                
a = esp_pbuf_new(10);                           /* Create pbuf with 10 bytes of memory */
b = esp_pbuf_new(20);                           /* Create pbuf with 20 bytes of memory */

esp_pbuf_chain(a, b);                           /* Link them together and increase reference count on b */

/*
 * When we are done using a pbuf, call free function.
 * This will free only pbuf a, as pbuf b has now 2 references:
 *  - one from pbuf a
 *  - one from variable b
 */
esp_pbuf_free(a);                               /* If we call this, it will free only first pbuf */ 
                                                /* As there is link to b pbuf somewhere */
a = NULL;                                       /* Reset a variable */

/*
 * At this point, b is still valid,
 * but when you don't need it anymore,
 * free it otherwise memory leak appears
 */
esp_pbuf_free(b);

b = NULL;                                       /* Reset b variable */