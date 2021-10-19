/**
 * \addtogroup      ESP_APP_HTTP_SERVER
 * \{
 *
 * \par             SSI (Server Side Includes) tags support
 *
 * SSI tags are supported on server to include user specific values as replacement of static content.
 *
 * Each tag must start with \ref HTTP_SSI_TAG_START tag and end with \ref HTTP_SSI_TAG_END and tag must not be longer than \ref HTTP_SSI_TAG_MAX_LEN.
 * White spaces are not allowed and `-` character is not allowed in tag name.
 * Example of valid tag is `\<!--#my_tag-->` (backslash is for escape only in this docs) where name of tag is `my_tag`.
 *
 * The tag name is later sent to SSI callback function where user can send custom data as tag replacement.
 *
 * \par             CGI (Common Gateway Interface) support
 *
 * CGI support allows you to hook different functions from clients to server.
 *
 *  - CGI paths must be enabled before with callback functions when CGI is triggered
 *  - CGI path must finish with `.cgi` suffix
 *      - To allow CGI hook, request URI must be in format `/folder/subfolder/file.cgi?param1=value1&param2=value2&`
 *
 * \par             Dynamic headers support
 *
 * Library can (based on request filename) decide what could be a good response type header.
 * When dynamic headers are active, server with parse input and will output up to 4 different header responses:
 *
 *  - Response code such as `HTTP/1.1 200 OK\r\n`
 *  - Server name defined with \ref HTTP_SERVER_NAME
 *  - Content-Length parameter output. This header is only pushed to output if SSI tags parsing is not included.
 *  - Content-Type and end of response delimiter. Content type is determined based on extension pairs written in HTTP server application. 
 *      Currently it supports: `HTML`, `TEXT`, `XML`, `PNG`, `JPG`, `JPEG`, `GIF`, `ICO`, `CSS`, `JS` but more of them can be added very easy
 *
 * \note            When SSI parsing is enabled for output, `Content-Length` part of headers is not included.
 *                  This is due to the fact that script does not know final output length as SSI tag outputs may have variable lengths.
 *                  Instead, `Content-Length` output will be included only in case SSI parsing is not active for specific request
 *
 * \par             HTTP server example with CGI and SSI
 *
 * \include         _example_http_server.c
 *
 * \}
 */