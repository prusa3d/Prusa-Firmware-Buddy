
#include <cstring>
#include "dbg.h"

#include "lwip/def.h"

#include "lwsapi.h"
#include "connection.hpp"

#define LIGHT_WSAPI_PORT          80
#define LIGHT_WSAPI_RETRIES       4 // 8 seconds timeout
#define LIGHT_WSAPI_POLL_INTERVAL 4 // once a two seconds

//! Full Bad Request response
static const char *http_bad_request = "HTTP/1.0 400 Bad Request\n"
                                      "Server: LwIP WSAPI\n"
                                      "Content-Type: text/plain\n"
                                      "\n"
                                      "Bed Request";

//! Full Internal Server Error response
static const char *http_internal_server = "HTTP/1.0 500 Internal Server Error\n"
                                          "Server: LwIP WSAPI\n"
                                          "Content-Type: text/plain\n"
                                          "\n"
                                          "Internal Server Error";

void Environment::add_header(IHeader *header) {
    if (headers == nullptr) {
        headers = header;
        last = headers;
    } else {
        last->next = header;
        last = header;
    }
}

//! Check when message is empty - all data was processed yet
static inline bool empty_message(const Message_t &msg) {
    return (msg.response == nullptr && msg.headers == nullptr && msg.length == 0);
}

static size_t lwsapi_write(Context *ctx, const uint8_t *data, size_t len);
static inline size_t lwsapi_write(Context *ctx, const char *data) {
    return lwsapi_write(ctx, (const uint8_t *)(data), strlen(data));
}

static err_t lwsapi_poll(void *arg, struct tcp_pcb *pcb);

//! Close connection and clean callbacks and other connection environment.
static err_t close_conn(struct tcp_pcb *pcb, Context *ctx = nullptr) {
    if (ctx != nullptr) {
        delete ctx;
    }

    tcp_arg(pcb, nullptr);
    tcp_recv(pcb, nullptr);
    tcp_err(pcb, nullptr);
    tcp_poll(pcb, nullptr, 0);
    tcp_sent(pcb, nullptr);

    tcp_output(pcb); // flush all data before close

    if (tcp_close(pcb) != ERR_OK) { // by doc, only ERR_MEM could be happend
        lwsapi_dbg("\tclose connetion fails, ABRT\n");
        tcp_abort(pcb);
        return ERR_ABRT;
    }

    return ERR_OK;
}

//! The poll function is called every 2nd second.
/*!
    If there has been no data sent (which resets the retries) in 8 seconds, close.
    If the last portion of a file has not been sent in 1 seconds, close.
    This could be increased, but we don't want to waste resources for bad
    connections. See
    http://www.nongnu.org/lwip/2_0_x/group__tcp__raw.html#gafba47015098ed7ce523dcf7bdf70f7e5

    @param arg pointer to connection context (Context)
 */
static err_t lwsapi_poll(void *arg, struct tcp_pcb *pcb) {
    if (arg == nullptr) {
        return close_conn(pcb);
    }

    Context *ctx = static_cast<Context *>(arg);
    ctx->retries++;
    if (ctx->retries == LIGHT_WSAPI_RETRIES) {
        lwsapi_dbg("\tmax retries, close\n");
        return close_conn(pcb, ctx);
    }

    return ERR_OK;
}

//! Process message from IResponse::generator, and call generator for next work
/*!
    While all data from Message_t is not processed, call lwsapi_write to
    response data to http client. This function is called first time after
    application_fn return IResponse, and next time from lwsapi_sent callback.

    When all data in Message_t is processed, generator is called. When
    Message_t.length is EOF, all data from generator is processed.
*/
static void lwsapi_call(Context *ctx, const struct pbuf *input = nullptr) {
    while (1) {
        // internal buffer and message was sent, call the coroutine
        if (ctx->buffer == nullptr && empty_message(ctx->message)) {
            ctx->message = ctx->response->generator(input);
            input = nullptr; // input must be processed only once
            ctx->m_position = 0;
            if (ctx->message.length == EOF) {
                close_conn(ctx->pcb, ctx);
                return;
            }
        }

        // try to send internal buffer if exists
        if (ctx->buffer != nullptr) {
            size_t size = strlen(ctx->buffer + ctx->m_position);
            size_t send = lwsapi_write(ctx,
                (const uint8_t *)ctx->buffer + ctx->m_position, size);
            if (send == size) {
                ctx->free_buffer();
                ctx->m_position = 0;
            } else if (send == 0) { // memory problem
                close_conn(ctx->pcb, ctx);
                return;
            } else { // not send all data, tcp buffer is full
                ctx->m_position += send;
                tcp_output(ctx->pcb);
                return;
            }
        }

        if (ctx->message.response != nullptr) {
            if (ctx->prepare_response() != ERR_OK) {
                close_conn(ctx->pcb, ctx);
            }
            continue;
        }

        if (ctx->message.headers != nullptr) {
            if (ctx->prepare_header() != ERR_OK) {
                close_conn(ctx->pcb, ctx);
            }
            continue;
        }

        if (ctx->message.length == 0) {
            return; // call later
        }

        size_t send = lwsapi_write(ctx, ctx->message.payload + ctx->m_position,
            ctx->message.length);
        if (send == 0 && ctx->message.length > 0) { // memory problem
            close_conn(ctx->pcb, ctx);
            return;
        }
        ctx->m_position += send;
        ctx->message.length -= send;
        if (ctx->message.length > 0) {
            tcp_output(ctx->pcb); // flush the data => free the buffer
            return;
        }
    }
}

//! Write data to TCP output buffer.
/*!
    While there is place in output TCP buffer, lwsapi_write copy data from
    Context.buffer to TCP buffer. When TCP buffer is full, stop.

    @param ctx pointer to connection context
    @param data pointer to array of data which must be copied to output tcp
           buffer
    @param len length of data, which must be copied
    @return size of data, which was be copied to output tcp buffer
*/
static size_t lwsapi_write(Context *ctx, const uint8_t *data, size_t len) {
    if (ctx == nullptr) {
        lwsapi_error("lwsapi_write: Bad input!\n");
        return 0;
    }

    size_t offset = 0;
    size_t max_len;
    size_t snd_len = len;
    while (offset < len) {
        /* We cannot send more data than space available in the send buffer. */
        max_len = tcp_sndbuf(ctx->pcb);
        if (max_len < snd_len) {
            snd_len = max_len;
        }

        if (max_len == 0) {
            return offset;
        }

        /* Additional limitation: e.g. don't enqueue more than 2*mss at once */
        max_len = ((u16_t)(2 * tcp_mss(ctx->pcb)));
        if (max_len < snd_len) {
            snd_len = max_len;
        }

        err_t err;
        do {
            /* Write packet to out-queue, but do not send it until tcp_output() is called. */
            err = tcp_write(ctx->pcb, data + offset, snd_len, TCP_WRITE_FLAG_COPY);
            if (err == ERR_MEM) {
                if ((tcp_sndbuf(ctx->pcb) == 0) || (tcp_sndqueuelen(ctx->pcb) >= TCP_SND_QUEUELEN)) {
                    /* no need to try smaller sizes */
                    snd_len = 1;
                } else {
                    snd_len /= 2;
                }
            }
        } while ((err == ERR_MEM) && (snd_len > 1));
        if (err == ERR_OK) {
            offset += snd_len;
            return offset;
        } else {
            lwsapi_dbg("[%p] lwsapi_write: tcp_write error: %d\n", ctx->pcb, err);
            return offset;
        }
    }

    return 0; // len is zero
}

#include "lwip/timeouts.h"

//! Process http request and call application.
/*!
    This is callback defined in LwIP documentation
    http://www.nongnu.org/lwip/2_0_x/group__tcp__raw.html#ga8afd0b316a87a5eeff4726dc95006ed0

    This function try to parse http request, fill Context attributes and call
    defined application_fn.

    TODO: there is missing process big http request - that means with more (big)
    headers, which could be more than 1024 bytes and request payload typical for
    POST|PUT|PATCH requests.

    @param arg is pointer to connection Context
*/
static err_t lwsapi_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    if ((err != ERR_OK) || (p == nullptr) || (arg == nullptr)) {
        /* error or closed by other side? */
        if (p != nullptr) {
            /* Inform TCP that we have taken the data. */
            tcp_recved(pcb, p->tot_len);
            pbuf_free(p);
        }
        return close_conn(pcb, static_cast<Context *>(arg));
    }

    /* Inform TCP that we have taken the data. */
    tcp_recved(pcb, p->tot_len);

    Context *ctx = static_cast<Context *>(arg);

    if (ctx->response.get() != nullptr) { // response exist yet
        ctx->retries = 0;

        lwsapi_call(ctx, p); // call the generator with input
        pbuf_free(p);
        return ERR_OK;
    }

    size_t eoh = 0; // offset from
    if (ctx->state == Context::State::FIRST) {
        ctx->state = Context::State::WAIT_FOR_EOH;
        eoh = ctx->find_eoh(p->payload, p->len); // try pbuf first
        if (eoh == 0) {
            if (ctx->fill_request_buffer(p) == ERR_MEM) {
                pbuf_free(p);
                lwsapi_write(ctx, http_internal_server);
                return close_conn(pcb, ctx);
            }
            eoh = ctx->find_eoh(); // use internal buffer
        }
    } else if (ctx->state == Context::State::WAIT_FOR_EOH) {
        ctx->fill_request_buffer(p); // append data to buffer
        eoh = ctx->find_eoh();       // use internal buffer
    }

    if (eoh > 0) // only set when not found in last callback
    {
        // when internal buffer is exist, will be used than instead of pbuf
        err_t rv = ctx->parse_request(p->payload, p->len);
        if (rv == ERR_VAL) {
            pbuf_free(p);
            lwsapi_write(ctx, http_bad_request);
            return close_conn(pcb, ctx);
        }
        if (rv == ERR_MEM) {
            pbuf_free(p);
            lwsapi_write(ctx, http_internal_server);
            return close_conn(pcb, ctx);
        }
        ctx->state = Context::State::PAYLOAD;
    }

    if (ctx->state != Context::State::PAYLOAD) // eoh not found yet
    {
        pbuf_free(p);
        return ERR_OK; // stop this call and wait for another recv
    }

    ctx->response = std::move(application(ctx->env));

    if (ctx->response.get() == nullptr) {
        pbuf_free(p);
        lwsapi_write(ctx, http_internal_server);
        return close_conn(pcb, static_cast<Context *>(arg));
    }

    struct pbuf *body;
    if (eoh > 0) // eoh found in this call, need to create shadow pbuf
    {
        eoh = ctx->find_eoh(p->payload, p->len) + 1; // find EOH in pbuf
        body = reinterpret_cast<struct pbuf *>(mem_malloc(sizeof(pbuf)));
        //FIXME: create valid shadow buffer chain
        body->payload = const_cast<void *> memshift(p->payload, eoh);
        body->len = p->len - eoh;
        body->tot_len = p->tot_len - eoh;
        body->next = p->next;
    } else {
        body = p;
    }

    lwsapi_call(ctx, body); // first try to call response generator
    if (body != p) {
        mem_free(body);
    }
    pbuf_free(p);
    return ERR_OK;
}

//! tcp_err callback defined in LwIP
static void lwsapi_err(void *arg, err_t err) {
    // lwsapi_error("lwsapi_err: (%d) %s\n", err, lwip_strerr(err));
    switch (err) {
    case ERR_RST:
        lwsapi_dbg("lwsapi_err: connection reset by remote host");
        break;
    case ERR_ABRT:
        lwsapi_dbg("lwsapi_err: connection aborted");
        break;
    default:
        lwsapi_error("lwsapi_err: err number %d", err);
    }

    if (arg != nullptr) {
        Context *ctx = static_cast<Context *>(arg);
        delete ctx;
    }
}

//! tcp_sent callback defined in LwIP
/*!
    This callback is called after http client confirms the part of tcp response.
    So next data could be send by lwsapi_call function.
 */
static err_t lwsapi_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    if (arg != nullptr) {
        Context *ctx = static_cast<Context *>(arg);
        ctx->retries = 0;

        lwsapi_call(ctx);
    }

    return ERR_OK;
}

//! tcp_accept callback defined in LwIP
/*!
    This function create connection Context, set it as new pcb argument,
    and set all callbacks for this new accepted connection. See
    http://www.nongnu.org/lwip/2_0_x/tcp_8h.html#a00517abce6856d6c82f0efebdafb734d

    @param pcb new protocol control block of new created http connection
*/
static err_t lwsapi_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    if ((err != ERR_OK) || (pcb == nullptr)) {
        return ERR_VAL;
    }

    tcp_setprio(pcb, TCP_PRIO_MIN);
    Context *ctx = new Context(pcb);
    if (ctx == nullptr) {
        lwsapi_error("lwsapi_accept: Out of memory, RST\n");
        return close_conn(pcb);
    }

    /* Set up the various callback functions */
    tcp_recv(pcb, lwsapi_recv);
    tcp_err(pcb, lwsapi_err);
    tcp_poll(pcb, lwsapi_poll, LIGHT_WSAPI_POLL_INTERVAL);
    tcp_sent(pcb, lwsapi_sent);

    return ERR_OK;
}

/*!
    Start LwIP WSAPI http server, which means set TCP priority for
    new protocol control block, bind on TCP port and set callback
    for accepting new connection.
*/
err_t lwsapi_init(void) {
    lwsapi_dbg("lwsapi: start\n");
    struct tcp_pcb *pcb;
    err_t err;

    pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (pcb == nullptr) {
        return ERR_MEM;
    }

    tcp_setprio(pcb, TCP_PRIO_MIN);
    err = tcp_bind(pcb, IP_ANY_TYPE, LIGHT_WSAPI_PORT);
    if (err != ERR_OK) {
        lwsapi_error("lwsapi: tcp_bind failed: %d", err);
        return err;
    }

    pcb = tcp_listen(pcb);
    if (pcb == nullptr) {
        lwsapi_error("lwsapi: tcp_listen failed");
        return ERR_CLSD;
    }

    tcp_accept(pcb, lwsapi_accept);
    return ERR_OK;
}

IHeader *dynamics_header_factory(const char *key,
    const char *value, size_t value_length) {
    return new DynamicsHeader(key, value, value_length);
}

IHeader *number_header_factory(const char *key,
    const char *value, size_t value_length) {
    return new NumberHeader(key, atoll(value));
}
