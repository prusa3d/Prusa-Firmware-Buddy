/**
 * \addtogroup      ESP_APP_HTTP_SERVER_FS_FAT
 * \{
 * 
 * This is FATFS implementation for HTTP server dynamic files.
 *
 * \note            More about FATFS can be found on its <a href="http://elm-chan.org/fsw/ff/00index_e.html" target="_blank">official website</a>.
 *
 * It consists of 3 functions, which must be applied to \ref http_init_t structure on server initialization:
 * 
 * - \ref http_fs_open to open a file
 * - \ref http_fs_read to read a file
 * - \ref http_fs_close to close a file
 *
 * When opening a file, functions assume user has files in <b>www</b> folder in root directory of file system.
 * 
 * \par             Example assigning file system functions to server
 *
 * \include 		_example_http_server_fatfs.c
 *
 * \}
 */