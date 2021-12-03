#include "lwsapi.h"
#include "string.h"

static Header_t header = { "Content-Type", "text/plain", NULL };

typedef struct {
    Header_t *header;
    const char *response;
    const char *payload;
    uint8_t done;
} Response_t;

//! Corroutine type funtion, is call while Message_t->length is not EOF
Message_t hello_world(void *arg) {
    if (arg == 0) {
        return (Message_t) { NULL, NULL, NULL, EOF };
    }
    Response_t *res = (Response_t *)arg;
    if (res->done) {
        free(res);
        return (Message_t) { NULL, NULL, NULL, EOF };
    }

    res->done = 1;
    return (Message_t) { res->response, res->header,
        (const uint8_t *)res->payload, strlen(res->payload) };
}

coroutine_fn application(Environment_t *env, void **arg) {
    Response_t *res = (Response_t *)calloc(1, sizeof(Response_t));
    res->response = "200 OK";
    res->header = &header;
    res->payload = "Hello world";

    *arg = (void *)res; // pointer to response is argument for couroutine fce
    return &hello_world;
}
