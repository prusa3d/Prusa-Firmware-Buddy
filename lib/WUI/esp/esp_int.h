#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#if !__DOXYGEN__
/**
 * \brief           Receive character structure to handle full line terminated with `\n` character
 */
typedef struct {
    char data[128]; /*!< Received characters */
    size_t len;     /*!< Length of valid characters */
} esp_recv_t;

    /* Receive character macros */
    #define RECV_ADD(ch)                                        \
        do {                                                    \
            if (recv_buff.len < (sizeof(recv_buff.data)) - 1) { \
                recv_buff.data[recv_buff.len++] = ch;           \
                recv_buff.data[recv_buff.len] = 0;              \
            }                                                   \
        } while (0)
    #define RECV_RESET()           \
        do {                       \
            recv_buff.len = 0;     \
            recv_buff.data[0] = 0; \
        } while (0)
    #define RECV_LEN()      ((size_t)recv_buff.len)
    #define RECV_IDX(index) recv_buff.data[index]

    /* Send data over AT port */
    #define AT_PORT_SEND_STR(str)       esp.ll.send_fn((const void *)(str), (size_t)strlen(str))
    #define AT_PORT_SEND_CONST_STR(str) esp.ll.send_fn((const void *)(str), (size_t)(sizeof(str) - 1))
    #define AT_PORT_SEND_CHR(str)       esp.ll.send_fn((const void *)(str), (size_t)1)
    #define AT_PORT_SEND_FLUSH()        esp.ll.send_fn(NULL, 0)
    #define AT_PORT_SEND(d, l)          esp.ll.send_fn((const void *)(d), (size_t)(l))
    #define AT_PORT_SEND_WITH_FLUSH(d, l) \
        do {                              \
            AT_PORT_SEND((d), (l));       \
            AT_PORT_SEND_FLUSH();         \
        } while (0)

    /* Beginning and end of every AT command */
    #define AT_PORT_SEND_BEGIN_AT()       \
        do {                              \
            AT_PORT_SEND_CONST_STR("AT"); \
        } while (0)
    #define AT_PORT_SEND_END_AT()         \
        do {                              \
            AT_PORT_SEND(CRLF, CRLF_LEN); \
            AT_PORT_SEND_FLUSH();         \
        } while (0)

    /* Send special characters over AT port with condition */
    #define AT_PORT_SEND_QUOTE_COND(q)        \
        do {                                  \
            if ((q)) {                        \
                AT_PORT_SEND_CONST_STR("\""); \
            }                                 \
        } while (0)
    #define AT_PORT_SEND_COMMA_COND(c)       \
        do {                                 \
            if ((c)) {                       \
                AT_PORT_SEND_CONST_STR(","); \
            }                                \
        } while (0)
    #define AT_PORT_SEND_EQUAL_COND(e)       \
        do {                                 \
            if ((e)) {                       \
                AT_PORT_SEND_CONST_STR("="); \
            }                                \
        } while (0)
#endif /* !__DOXYGEN__ */






#ifdef __cplusplus
}
#endif /* __cplusplus */

