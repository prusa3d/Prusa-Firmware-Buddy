const void* data;
size_t pos, len;
lwesp_pbuf_p a, b, c;

const char str_a[] = "This is one long";
const char str_a[] = "string. We want to save";
const char str_a[] = "chain of pbufs to file";

/* Create pbufs to hold these strings */
a = lwesp_pbuf_new(strlen(str_a));
b = lwesp_pbuf_new(strlen(str_b));
c = lwesp_pbuf_new(strlen(str_c));

/* Write data to pbufs */
lwesp_pbuf_take(a, str_a, strlen(str_a), 0);
lwesp_pbuf_take(b, str_b, strlen(str_b), 0);
lwesp_pbuf_take(c, str_c, strlen(str_c), 0);

/* Connect pbufs together */
lwesp_pbuf_chain(a, b);
lwesp_pbuf_chain(a, c);

/*
 * pbuf a now contains chain of b and c together
 * and at this point application wants to print (or save) data from chained pbuf
 *
 * Process pbuf by pbuf with code below
 */

/*
 * Get linear address of current pbuf at specific offset
 * Function will return pointer to memory address at specific position
 * and `len` will hold length of data block
 */
pos = 0;
while ((data = lwesp_pbuf_get_linear_addr(a, pos, &len)) != NULL) {
    /* Custom process function... */
    /* Process data with data pointer and block length */
    process_data(data, len);
    printf("Str: %.*s", len, data);

    /* Increase offset position for next block */
    pos += len;
}

/* Call free only on a pbuf. Since it is chained, b and c will be freed too */
lwesp_pbuf_free(a);
