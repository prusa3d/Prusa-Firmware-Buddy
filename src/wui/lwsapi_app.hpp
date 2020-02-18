//! LwIP WSAPI C/C++ implementation.
/** This is C/C++ implementation of Lua's WSAPI or prospal related to WSGI 2.0,
    for LwIP stack, which is base on tcp callbacks. The interface is like
    Lua's WSAPI, but extend to context pointers, while C don't have coroutines,
    and LwIP could work on more systems, so context switching could not be
    good idea.

    +--------+     +----------------------------------+  |
    |Thread X|     | LwIP Thread                      |  |
    |        |     | +-------------------------------+|  |
    |        |     | |LwIP                           ||  |
    |        |     | |+-------------------+  +-----+ ||  |  +-------------+
    |        |     | || WSAPI HTTP Server <--> Eth <--------> HTTP Client |
    |        |     | |+----------^--------+  +-----+ ||  |  +-------------+
    |        |     | +-----------|-------------------+|  |
    |+------+|     | +-----------v------------+       |  |
    || Data <--------> WSAPI HTTP Application |       |  |
    |+------+|     | +------------------------+       |  |
    +--------+     +----------------------------------+  |
*/

#pragma once

#include <memory>
#include <cstring>

#include "lwip/pbuf.h"
#include "lwip/mem.h"

#include "dbg.h"

//! maximum of HTTP request without payload
#define MAX_HTTP_REQUEST 1024

//! maximum method length (PROPPATCH) from WebDAV + \0
#define METHOD_LENGTH 10

//! maximum length of request uri
#define URI_LENGTH 64

#define lwsapi_dbg   _dbg
#define lwsapi_error _dbg

#define lwsapi_free(arg)  \
    if (arg != nullptr) { \
        mem_free(arg);    \
        arg = nullptr;    \
    }

/*!
    All classes, which could be dynamical initialized in LwIP thread, must
    do internal memory pool, or another thread safe allocation. This class
    have own new/delete operators, which create object instance in LwIP memory
    pool.
*/
class LwIPClass {
public:
    static void *operator new(size_t size) {
        return mem_malloc(size);
    }

    static void operator delete(void *ptr) {
        return mem_free(ptr);
    }

    virtual ~LwIPClass() {}
};

//! Headers list. Creator is responsible to clean the values.
class IHeader : public LwIPClass {
public:
    const char *key; /**< key must be const char defined in code */
    IHeader *next;

    IHeader(const char *const key, IHeader *next = nullptr)
        : key(key)
        , next(next) {}

    virtual size_t length() const = 0;
    virtual void snprintf(char *buff) const = 0;
    virtual void dbg() const = 0;

protected:
    static const size_t format_chars = 5; // ': \r\n\0'
};

//! NumberHeader is for number headers like Content-Length
class NumberHeader : public IHeader {
public:
    size_t value;

    NumberHeader(const char *const key, size_t value, IHeader *next = nullptr)
        : IHeader(key, next)
        , value(value) {}

    virtual size_t length() const override {
        size_t count = 1;
        size_t tmp = value;
        while (tmp > 9) {
            count++;
            tmp = tmp / 10;
        }

        return count + strlen(key) + format_chars;
    }
    virtual void snprintf(char *buff) const override {
        ::snprintf(buff, length(), "%s: %zu\r\n", key, value);
    }

    virtual void dbg() const override {
        lwsapi_dbg("Header: [%s]:'%zu'", key, value);
    }
};

//! ConstHeader only point to const chars defined in code
class ConstHeader : public IHeader {
public:
    const char *value;

    ConstHeader(const char *const key, const char *const value,
        IHeader *next = nullptr)
        : IHeader(key, next)
        , value(value) {}

    virtual size_t length() const override {
        return strlen(key) + strlen(value) + format_chars;
    }

    virtual void snprintf(char *buff) const override {
        ::snprintf(buff, length(), "%s: %s\r\n", key, value);
    }

    virtual void dbg() const override {
        lwsapi_dbg("Header: [%s]:'%s'", key, value);
    }
};

//! DynamicsHeader store it's value to LwIP memory pool
class DynamicsHeader : public IHeader {
public:
    char *value;

    DynamicsHeader(const char *const key, const char *value,
        IHeader *next = nullptr)
        : IHeader(key, next) {
        this->value = reinterpret_cast<char *>(
            mem_calloc(sizeof(char), strlen(value) + 1));
        if (this->value != nullptr) {
            memcpy(this->value, value, strlen(value));
        }
    }

    DynamicsHeader(const char *const key, const char *value, size_t length,
        IHeader *next = nullptr)
        : IHeader(key, next) {
        this->value = reinterpret_cast<char *>(
            mem_calloc(sizeof(char), length + 1));
        if (this->value != nullptr) {
            memcpy(this->value, value, length);
        }
    }

    ~DynamicsHeader() {
        lwsapi_free(value);
    }

    virtual size_t length() const override {
        return strlen(key) + strlen(value) + format_chars;
    }

    virtual void snprintf(char *buff) const override {
        ::snprintf(buff, length(), "%s: %s\r\n", key, value);
    }

    virtual void dbg() const override {
        lwsapi_dbg("Header: [%s]:'%s'", key, value);
    }
};

//! Environment struct like as WSGI environment as possible could be.
class Environment {
public:
    char method[METHOD_LENGTH] = { '\0' };   /**< HTTP METHOD (GET|POST|etc..) */
    char request_uri[URI_LENGTH] = { '\0' }; /**< Full HTTP request uri */

    Environment()
        : headers(nullptr)
        , last(nullptr) {}

    ~Environment() {
        auto it = headers;
        IHeader *next = nullptr;
        while (it != nullptr) {
            next = it->next;
            delete it;
            it = next;
        }
    }

    void add_header(IHeader *header);
    const IHeader *get_headers() const {
        return headers;
    }

private:
    IHeader *headers; /**< List of request headers */
    IHeader *last;
};

//! Message which must be returned from coroutine generator.
struct Message_t {
    const char *response;   /**< 200 OK etc.*/
    const IHeader *headers; /**< response header */
    const uint8_t *payload;
    int length; /**< payload length */
};

class IResponse : public LwIPClass {
public:
    IResponse(const IResponse &) = delete;
    IResponse() {}

    //! WSAPI generator (called more time from WSAPI http server).
    /*!
	   This is generator method must iterative return response content.

	   @param input is LwIP input buffer chain.

	   @return Message contains response, headers, payload and length. If
            length is EOF, all data was sent. Response must be set in first
            time, headers second time and payload could be send moretimes. When
            response or headers in message exists, that will be send. All data
            in message must exists to next generator call!
	*/
    virtual Message_t generator(const struct pbuf *input = nullptr) = 0;

    typedef std::unique_ptr<IResponse> unique_ptr_t;
};

//! application_fn typedef, which is called in tcp_recv callback.
/*!
   This is "WSGI" application handler, which gets reference to Environment
   structure. This must return IResponse::unique_ptr_t, which must implements
   generator method to return response content iterative if is needed.
*/
typedef IResponse::unique_ptr_t(application_fn)(Environment &env);

//! Define of application functions
IResponse::unique_ptr_t application(Environment &env);

typedef IHeader *(*header_factory_fn)(const char *key,
    const char *value, size_t value_length);

//! Return new ConstHeader
IHeader *const_header_factory(const char *key,
    const char *value, size_t value_length);

//! Return new DynamicsHeader
IHeader *dynamics_header_factory(const char *key,
    const char *value, size_t value_length);

//! Response new NumberHeader
IHeader *number_header_factory(const char *key,
    const char *value, size_t value_length);

//! This factory is used for parsing input headers
/*!
   request_header_fn is called when each header was detected in request.
   Application part of this http server must implement this function to
   decide which header is needed, and which not. All other headers will be
   ignored.

   @return Right header object or nullptr when header could be ignore
*/
typedef IHeader *(request_header_fn)(const char *key, size_t key_length,
    const char *value, size_t value_length);

IHeader *request_header(const char *key, size_t key_length,
    const char *value, size_t value_length);
