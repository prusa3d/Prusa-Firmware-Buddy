esp_pbuf_p a, b;
                                                
a = esp_pbuf_new(10);                           /* Create pbuf with 10 bytes of memory */ 
b = esp_pbuf_new(20);                           /* Create pbuf with 20 bytes of memory */ 

esp_pbuf_cat(a, b);                             /* Link them together */ 

/*
 * From now on, operating with b variable has undefined behavior, we should not use it anymore.
 *
 * The best way would be to set b reference to NULL and never use it again
 */
b = NULL;

/*
 * When done with memory, free only a
 * which will also free next linked in a list
 */
esp_pbuf_free(a);                               /* If we call this, it will free pbuf for a and b together */ 