char hostname[20];

/* Hostname event function, called when lwesp_hostname_get() function finishes */
void
hostname_fn(lwespr_t res, void* arg) {
    /* Check actual result from device */
    if (res == lwespOK) {
        printf("ESP hostname is %s\r\n", hostname);
    } else {
        printf("Error reading ESP hostname...\r\n");
    }
}

/* Somewhere in thread and/or other ESP event function */

/* Get device hostname in non-blocking mode */
/* Function now returns if command has been sent to internal message queue */
if (lwesp_hostname_get(hostname, sizeof(hostname), hostname_fn, NULL, 0 /* 0 means non-blocking call */) == lwespOK) {
    /* At this point application knows that command has been sent to queue */
    /* But it does not have yet valid data in "hostname" variable */
    printf("ESP hostname get command sent to queue.\r\n");
} else {
    /* Error writing message to queue */
    printf("Cannot send hostname get command to queue.\r\n");
}