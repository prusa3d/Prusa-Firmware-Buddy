/*
 * Initialization parameters for HTTP server with FAT FS file system
 */
const http_init_t
http_init = {
    /* Set other parameters such as SSI tags or CGI handlers */

    .fs_open = http_fs_open,                    /* Set open function */
    .fs_read = http_fs_read,                    /* Set read function */
    .fs_close = http_fs_close,                  /* Set close function */
};

/* Later somewhere, when you init http_server, call: */
esp_http_server_init(&http_init, 80);           /* Enable server on port 80 */