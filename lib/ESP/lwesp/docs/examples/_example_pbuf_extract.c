const void* data;
size_t pos, len;

esp_pbuf_p a, b, c;
const char str_a[] = "This is one long";
const char str_a[] = "string. We want to save";
const char str_a[] = "chain of pbufs to file";

/* Create pbufs to hold these strings */
a = esp_pbuf_new(strlen(str_a));
b = esp_pbuf_new(strlen(str_b));
c = esp_pbuf_new(strlen(str_c));

/* Write data to pbufs */
esp_pbuf_take(a, str_a, strlen(str_a), 0);
esp_pbuf_take(b, str_b, strlen(str_b), 0);
esp_pbuf_take(c, str_c, strlen(str_c), 0);

/* Connect pbufs together */
esp_pbuf_chain(a, b);
esp_pbuf_chain(a, c);

/*
 * pbuf a now contains chain of b and c together
 * and at this point we want to print (or save) data from chained pbuf
 *
 * Process pbuf by pbuf
 */
pos = 0;
do {
	/*
	 * Get linear address of current pbuf at specific offset
	 * Function will return pointer to memory address at specific position
	 * and will save length of data block
	 */
	data = esp_pbuf_get_linear_addr(a, pos, &len);
	if (data != NULL) {
		/* Custom process function... */
		process_data(data, len);				/* Process data with data pointer and block length */
		printf("Str: %.*s", len, data);
		pos += len;								/* Increase offset position for next block */
	}
} while (data != NULL);

/* Call free only on a pbuf. Since we have chain, b and c will be freed aswell */
esp_pbuf_free(a);
