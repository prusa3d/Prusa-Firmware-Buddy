#include <http/websocket.hpp>

#include <catch2/catch.hpp>
#include <cstring>

using namespace http;

namespace {

class TestKey final : public WebSocketKey {
public:
    // Values borrowed from https://en.wikipedia.org/wiki/WebSocket
    TestKey() {
        strcpy(request, "x3JJHMbDL1EzLkh9GBhXDw==");
        compute_response();
        REQUIRE(strcmp(response, "HSmrc0sMlYUkAGmm5OPpG2HaGWk=") == 0);
    }
};

} // namespace

TEST_CASE("Websocket key exchange") {
    TestKey test_key; // Test done in the constructor
}
