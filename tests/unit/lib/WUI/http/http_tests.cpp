#include <cstdint>
#include <cerrno>
#include <http/httpd.h>

#include <catch2/catch.hpp>
#include <memory>

using std::unique_ptr;

namespace {

class MockHandler : public HttpHandlers {};

class MockServer {
private:
    unique_ptr<MockHandler> handlers;
    unique_ptr<struct altcp_pcb, decltype(&httpd_free)> server;

public:
    MockServer()
        : handlers(new MockHandler())
        , server(httpd_new(handlers.get(), 666), httpd_free) {
    }
};

}

TEST_CASE("http tests") {
    MockServer server;
}
