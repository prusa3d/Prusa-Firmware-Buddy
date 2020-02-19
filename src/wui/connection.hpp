#pragma once

#include "lwip/tcp.h"

#include "lwsapi_app.hpp"

#define CRLF                   "\r\n"
#define MINIMAL_REQUEST_LENGTH 14 // GET / HTTP/1.0

#define memshift(ptr, size) \
    (reinterpret_cast<const void *>(reinterpret_cast<size_t>(ptr) + size))
#define chrshift(ptr, size) \
    (reinterpret_cast<const char *>(reinterpret_cast<size_t>(ptr) + size))

//! return lenght of data from start to end
#define memlen(start, end) \
    (reinterpret_cast<size_t>(end) - reinterpret_cast<size_t>(start) + 1)

//! Internal connection structure which is used in LwIP tcp_ callbacks as arg.
class Context : public LwIPClass {
public:
    enum State {
        FIRST,        // firt call of recv callback after constructor
        WAIT_FOR_EOH, // wait for end of header \r\n\r\n
        PAYLOAD       // pbuf containt payload
    };

    struct tcp_pcb *pcb;
    uint8_t retries; /*< retries >*/
    State state;
    Environment env;   /**< Request WSAPI environment */
    Message_t message; /**< Last message returned from application */
    char *buffer;      /**< Buffer for internal output data */
    size_t m_position; /**< Position of data in last message which is
                                     write not yet. */
    size_t end_of_header;
    IResponse::unique_ptr_t response; /**< Response object with generator function. */

    static void *operator new(size_t size) noexcept {
        if (Context::instances == Context::MAX) {
            lwsapi_error("Context::new Too many connections\n");
            return nullptr;
        }
        void *rv = mem_malloc(size);
        if (rv != nullptr) {
            Context::instances++;
        }
        return rv;
    }

    static void operator delete(void *ptr) {
        Context::instances--;
        mem_free(ptr);
    }

    Context(struct tcp_pcb *pcb)
        : pcb(pcb)
        , retries(0)
        , state(FIRST)
        , message({ nullptr, nullptr, nullptr, 0 })
        , buffer(nullptr)
        , m_position(0)
        , end_of_header(0)
        , response(nullptr)
        , request_buffer(nullptr)
        , request_buffer_size(0) {
        tcp_arg(pcb, this);
    }

    ~Context() {
        lwsapi_free(buffer);         // when buffer is not deleted
        lwsapi_free(request_buffer); // when request_buffer is not deleted
        tcp_arg(this->pcb, nullptr);
    }

    //! Append pbuf to request_buffer
    err_t fill_request_buffer(const struct pbuf *p);

    //! Try to find End Of Header (\r\n\r\n) in buffer
    /*!
	    @return position in data of \r\n\r\n. Return value can't
	        be less then MINIMAL_REQUEST_LENGTH. If EOH sequence is not
	        found, zero is returned.
	*/
    size_t find_eoh(const void *data = nullptr, size_t length = 0);

    //! parste the request header (request line + request headers)
    err_t parse_request(const void *data = nullptr, size_t length = 0);

    err_t prepare_response();

    err_t prepare_header();

    inline void free_buffer() {
        lwsapi_free(buffer);
    }

private:
    static const uint8_t MAX = 4;
    static uint8_t instances;

    char *request_buffer; /**< will be used when request can't be parsed at once */
    uint16_t request_buffer_size;

    //! parse request line GET / HTTP/1.0
    err_t parse_request_line(const void *line, size_t length);

    //! parse request header and add to env headers chain
    err_t parse_header(const void *line, size_t length);
};
