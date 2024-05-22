#include "httpc.hpp"
#include "connection.hpp"
#include <socket.hpp>
#include <iostream>

int main() {
    char url[] = { "dev.ct.xln.cz" };
    uint16_t port_no = 80;
    // check for data to send to connect server
    constexpr size_t client_buffer_len = 512;
    uint8_t client_buffer[client_buffer_len];
    class socket_con conn;
    http_client client { url, port_no, &conn };

    std::optional<Error> ret;

    REQUEST_TYPE type = REQUEST_TYPE::SEND_INFO;
    char fingerprint[] = "kllkhlkhlhlhlhl";
    char token[] = "kkhkhkhkhk";
    ret = client.send_header(type, fingerprint, token, 156);
    if (ret.has_value()) {
        std::cout << "error send header" << std::endl;
    }

    type = REQUEST_TYPE::TELEMETRY;
    char fingerprint1[50] = "kllkhlkhlhlhlhl";
    char token1[50] = "kkhkhkhkhk";
    ret = client.send_header(type, fingerprint1, token1, 156);
    if (ret.has_value()) {
        std::cout << "error send header" << std::endl;
    }
}
