#include "lwsapi_app.hpp"
#include "http_states.h"
#include "dbg.h"
#include "wui.h"

#include <cstring>
#include <cstdarg>

#ifndef TEST_INTEGRITY
    #include "../Marlin/src/module/temperature.h"
#endif

#include "filament.h"
#include "window_header.h"
#include "status_footer.h"

extern marlin_vars_t* webserver_marlin_vars;

#define X_AXIS_POS 0
#define Y_AXIS_POS 1
#define Z_AXIS_POS 2

#ifdef RFC1123_DATETIME
    #define LAST_MODIFY RFC1123_DATETIME
#else
    #define LAST_MODIFY __TIMESTAMP__
#endif

#include "res/cc/index_html.c"
#include "res/cc/index_css.c"
#include "res/cc/index_js.c"
#include "res/cc/favicon_ico.c"
#include "res/cc/connect_black_svg.c"

#include "res/cc/status_filament_svg.c"
#include "res/cc/status_heatbed_svg.c"
#include "res/cc/status_nozzle_svg.c"
#include "res/cc/status_prnflow_svg.c"
#include "res/cc/status_prnspeed_svg.c"
#include "res/cc/status_z_axis_svg.c"
#include "res/cc/under_construction_gif.c"

struct header_factory_t {
    const char *key;
    const header_factory_fn factory;
};

const header_factory_t req_headers[] = {
    { "Host", &dynamics_header_factory },
    { "Content-Type", &dynamics_header_factory },
    { "Content-Length", &number_header_factory },
};

//! File handler structure
struct FileHandler_t {
    const char *file_name;
    const char *content_type;
    int content_length;
    const uint8_t *data;
};

//! Array of files which could be responed by http
static const FileHandler_t files[] = {
    { "/", "text/html", sizeof(index_html), index_html },
    { "/index.css", "text/css", sizeof(index_css), index_css },
    { "/index.js", "application/javasript", sizeof(index_js), index_js },
    { "/favicon.ico", "image/x-icon", sizeof(favicon_ico), favicon_ico },
    { "/connect_black.svg", "image/svg+xml", sizeof(connect_black_svg),
        connect_black_svg },
    { "/status_filament.svg", "image/svg+xml", sizeof(status_filament_svg),
        status_filament_svg },
    { "/status_heatbed.svg", "image/svg+xml", sizeof(status_heatbed_svg),
        status_heatbed_svg },
    { "/status_nozzle.svg", "image/svg+xml", sizeof(status_nozzle_svg),
        status_nozzle_svg },
    { "/status_prnflow.svg", "image/svg+xml", sizeof(status_prnflow_svg),
        status_prnflow_svg },
    { "/status_prnspeed.svg", "image/svg+xml", sizeof(status_prnspeed_svg),
        status_prnspeed_svg },
    { "/status_z_axis.svg", "image/svg+xml", sizeof(status_z_axis_svg),
        status_z_axis_svg },
    { "/under_construction.gif", "image/gif", sizeof(under_construction_gif),
        under_construction_gif },
};

//! Response state enumeration
enum State_t {
    FIRST, /**< First coroutine_fn call after creation */
    NEXT, /**< Any other coroutine_fn call while all data is not sent */
    DONE /**< Last coroutine_fn call, coroutine_fn must delete response
                     object */
};

//! Class to response any static files defined in files array.
class FileResponse : public IResponse {
    const FileHandler_t *handler;
    ConstHeader ct_header; /**< Content-Type header is firt in chain */
    NumberHeader cl_header;
    ConstHeader lm_header;
    ConstHeader cc_header;
    bool done;
    static constexpr const char *last_modified = LAST_MODIFY;

public:
    FileResponse(const FileHandler_t *handler)
        : handler(handler)
        , ct_header("Content-Type", handler->content_type, &cl_header)
        , cl_header("Content-Length", handler->content_length, &lm_header)
        , lm_header("Last-Modify", last_modified, &cc_header)
        , cc_header("Cache-Control", "public, max-age=31536000")
        , done(false) {}

    //! Response generator, iterative returns file data
    virtual Message_t generator(const struct pbuf *input = nullptr) override;
};

Message_t FileResponse::generator(const struct pbuf *input) {
    Message_t msg = { nullptr, nullptr, nullptr, EOF };

    if (!done) {
        msg = (Message_t) { HTTP_200, &ct_header, handler->data, handler->content_length };
        done = true;
    }
    return msg;
}

#define BUFFER_RESPONSE_SIZE 256

//! Universal response class with internal buffer size by BUFFER_RESPONSE_SIZE
class BufferResponse : public IResponse {
public:
    const char *response;
    ConstHeader ct_header;
    bool done;

    BufferResponse()
        : response(nullptr)
        , ct_header("Content-Type", "application/binary")
        , done(false) {}

    //! write string data to internal buffer
    int printf(const char *format...) {
        int rv = 0;
        va_list args;
        va_start(args, format);
        rv = vsnprintf(_buffer, BUFFER_RESPONSE_SIZE, format, args);
        va_end(args);
        return rv;
    }

    //! return internal string buffer as array of bytes
    const uint8_t *buffer() const {
        return reinterpret_cast<const uint8_t *>(_buffer);
    }

    //! return length of internal string buffer
    int length() const {
        return strnlen(_buffer, BUFFER_RESPONSE_SIZE);
    }

    //! Response generator, iterative returns file data
    virtual Message_t generator(const struct pbuf *input = nullptr) override;

private:
    char _buffer[BUFFER_RESPONSE_SIZE] = { '\0' };
};

class WaitResponse : public IResponse {
public:
    size_t done;
    size_t request_length;
    bool close;

    WaitResponse(size_t request_length)
        : done(0)
        , request_length(request_length)
        , close(false) {}

    virtual Message_t generator(const struct pbuf *input = nullptr) override {
        for (auto it = input; it != nullptr; it = it->next) {
            for (size_t i = 0; i < it->len; i++) {
                // process data
                // printf("%c", ((char*)(it->payload))[i]);
            }
            done += it->len;
        }

        if (close) {
            return (Message_t) { nullptr, nullptr, nullptr, EOF };
        }
        if (done == request_length) {
            close = true;
            return (Message_t) { HTTP_200, nullptr, nullptr, 0 };
        }

        // not readed
        return (Message_t) { nullptr, nullptr, nullptr, 0 };
    }
};

//! coroutine_fn returned BufferResponse data
/*!
     @param arg pointer to BufferResponse object, which must be application
        function, and must be deleted by this function
*/
Message_t BufferResponse::generator(const struct pbuf *input) {
    //const char* dummny = "This is dummy data. ";
    Message_t msg = { nullptr, nullptr, nullptr, EOF };

    if (!done) {
        msg = (Message_t) { response, &ct_header, buffer(), length() };
        done = true;
    }
    return msg;
}

//! return coroutine_fn for /api/job request
/*!
    Function create BufferResponse object, set its pointer to arg and
    return buffer_coroutine.
*/
IResponse::unique_ptr_t api_job(Environment &env) {
    std::unique_ptr<BufferResponse> res(new BufferResponse());
    const char* file_name = "test.gcode";
    uint8_t sd_percent_done = (uint8_t)(webserver_marlin_vars->sd_percent_done);
    uint32_t print_duration = (uint32_t)(webserver_marlin_vars->print_duration);
    res->printf("{"
            "\"file\":\"%s\","
            "\"total_print_time\":%d, "
            "\"progress\":{\"precent_done\":%d}"
            "}",
            file_name,
            print_duration,
            sd_percent_done
            );
    return std::move(res);
}

//! return coroutine_fn for /api/printer request
/*!
    Function create BufferResponse object, set its pointer to arg and
    return buffer_coroutine.
*/
IResponse::unique_ptr_t api_printer(Environment &env) {
    std::unique_ptr<BufferResponse> res(new BufferResponse());
    if (res.get() != nullptr) {
        res->response = HTTP_200;
        res->ct_header.value = "application/json";

        int32_t actual_nozzle = (int32_t)(webserver_marlin_vars->temp_nozzle);
        int32_t target_nozzle = (int32_t)(webserver_marlin_vars->target_nozzle);
        int32_t actual_heatbed = (int32_t)(webserver_marlin_vars->temp_bed);
        int32_t target_heatbed = (int32_t)(webserver_marlin_vars->target_bed);

        double x_pos_mm = static_cast <double> (webserver_marlin_vars->ipos[X_AXIS_POS]);
        double y_pos_mm = static_cast <double> (webserver_marlin_vars->ipos[Y_AXIS_POS]);
        double z_pos_mm = static_cast <double> (webserver_marlin_vars->ipos[Z_AXIS_POS]);
        uint16_t print_speed = (uint16_t) (webserver_marlin_vars->print_speed);
        uint16_t flow_factor = (uint16_t) (webserver_marlin_vars->flow_factor);
        const char* filament_material = filaments[get_filament()].name;

        res->printf(
                "{\"temperature\":{"
                        "\"tool0\":{\"actual\":%d, \"target\":%d},"
                        "\"bed\":{\"actual\":%d, \"target\":%d}},"
                 "\"xyz_pos_mm\":{"
                         "\"x\":%.2f, \"y\":%.2f, \"z\":%.2f},"
                 "\"print_settings\":{"
                         "\"printing_speed\":%hd, \"flow_factor\":%hd, \"filament_material\":\"%s\"} }",

                     actual_nozzle, target_nozzle,
                     actual_heatbed, target_heatbed,
                     x_pos_mm, y_pos_mm, z_pos_mm,
                     print_speed, flow_factor,
                     filament_material
                  );
    }
    return std::move(res);
}

//! return 200 OK when file is post right
/*!
*/
IResponse::unique_ptr_t api_post(Environment &env) {
    printf("Request headers:\n");
    size_t content_length = 0;
    for (auto it = env.get_headers(); it != nullptr; it = it->next) {
        char buff[80];
        it->snprintf(buff); // TODO: volitelna velikost bufferu :D
        printf(buff);
        if (!strcmp(it->key, "Content-Length")) {
            content_length = dynamic_cast<const NumberHeader *>(it)->value;
        }
    }

    std::unique_ptr<WaitResponse> res(new WaitResponse(content_length));
    return res;
}
//! return coroutine_fn for Not Found response
/*!
    Function create BufferResponse object, set its pointer to arg and
    return buffer_coroutine.
*/
IResponse::unique_ptr_t not_found(Environment &env) {
    std::unique_ptr<BufferResponse> res(new BufferResponse());
    if (res.get() != nullptr) {
        res->response = HTTP_404;
        res->ct_header.value = "text/plain";

        res->printf("Not Found");
    }
    return res;
}

//! request_header_fn callbacke, which is call from LwIP WSAPI http server
IHeader *request_header(const char *key, size_t key_length,
    const char *value, size_t value_length) {
    for (size_t i = 0; i < sizeof(req_headers) / sizeof(header_factory_t); i++) {
        if (!std::strncmp(req_headers[i].key, key, key_length)) {
            return req_headers[i].factory(req_headers[i].key, value, value_length);
        }
    }
    return nullptr;
}

//! application_fn callback, which is called from LwIP WSAPI http server
/*!
    @param env request environment defined in lwsapi.h
    @param arg pointer to pointer to internal application reqeust object. This
        pointer is argument of coroutine_fn funtcions.
    @return coroutine_fn for response
*/
IResponse::unique_ptr_t application(Environment &env) {

    _dbg("HTTP Request: %s %s", env.method, env.request_uri);

    if (!strcmp(env.method, "GET") || !strcmp(env.method, "HEAD")) {
        // Static files
        for (size_t i = 0; i < (sizeof(files) / sizeof(FileHandler_t)); i++) {
            if (!strcmp(env.request_uri, files[i].file_name)) {
                return std::unique_ptr<FileResponse>(new FileResponse(&files[i]));
            }
        }

        if (!strcmp(env.request_uri, "/api/job")) {
            return api_job(env);
        } else if (!strcmp(env.request_uri, "/api/printer")) {
            return api_printer(env);
        }
    } else if (!strcmp(env.method, "POST")) {
        if (!strcmp(env.request_uri, "/post")) {
            return api_post(env);
        }
    }
    return not_found(env);
}
