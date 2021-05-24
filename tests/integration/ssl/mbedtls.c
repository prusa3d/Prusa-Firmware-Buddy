#include "mbedtls.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/certs.h"
#include "mbedtls/platform.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "cmsis_os.h"
#include "dbg.h"
#include <string.h>

#define SERVER_PORT "1234"
#define SERVER_NAME "10.25.234.204"
#define DEBUG_LEVEL 3
unsigned char memory_buf[40000];

#define CERT_RSA_2048                                                    \
    "-----BEGIN CERTIFICATE-----\n"                                      \
    "MIICzzCCAbegAwIBAgIJAIlUw1YxLh10MA0GCSqGSIb3DQEBBQUAMBgxFjAUBgNV\n" \
    "BAMTDTEwLjI1LjIzNC4yMDQwHhcNMjEwNTI0MDcyOTAwWhcNMzEwNTIyMDcyOTAw\n" \
    "WjAYMRYwFAYDVQQDEw0xMC4yNS4yMzQuMjA0MIIBIjANBgkqhkiG9w0BAQEFAAOC\n" \
    "AQ8AMIIBCgKCAQEA08NuPy26LkHAe0MmCGoyMQwtR6xy42itB3v1vTsCi4LQ0CDE\n" \
    "hkiP7kqzoglDnYl8FGbpdA54ViWdiuQ1GcR9BZcnvlWfQCgCE+fVUc5TLewHM4hl\n" \
    "BTFFNaSEbj3S9pscykqkvzesyJMuhFGwLBm5dDaDUyMwqyNPSIyy6AkH66ButteS\n" \
    "AMt09dGb6Y6WYeq7udFXT0M/fYQpr2HKd/dqNrYlE8ixQknnBPK7u4AR7Jj2dr01\n" \
    "L2rgdn82tbVzJBpmVvJgNfe1eD3SYSLR/kj4OHcTEwjxHpVkEFdKqF+6xrKZQygn\n" \
    "3tX1XzxUY3518SpU5FA7Nk+opp+v1Ek/uVm3nwIDAQABoxwwGjAYBgNVHREEETAP\n" \
    "gg0xMC4yNS4yMzQuMjA0MA0GCSqGSIb3DQEBBQUAA4IBAQCXexAKNsWJ6af/1rI8\n" \
    "QwVwUcpJPKWSB0KoVD73h6udUIq3MswKsZ4kUe2Ii5kbXRnTZhctckTwA5XLa8IA\n" \
    "S3KpSFtq2N8tjVRSz+dQoZTyOicuC09waTqRXvYDKnAe1yE0dBuX5748lM3OSEHc\n" \
    "WYFZ9JCUMLrEhmeZi+NCSeeMP03+meJnoX9DrV6mhJkMqh4ud9/sHA0Tml3LN+sG\n" \
    "FRj+8YN/F8X/yTA0RC65Iqa19wo6tSkQffpDIfU75hwWqNO0624uwfnfEQZlCi7+\n" \
    "s0d9Fj4OoNmlB44qPBq9Pfk0bY3kjgde4iFeYtJxMOZb6gDTB/ZlDqQHN5m1//LO\n" \
    "fVrs\n"                                                             \
    "-----END CERTIFICATE-----\n"

#define CERT_RSA_1024                                                    \
    "-----BEGIN CERTIFICATE-----\n"                                      \
    "MIICVDCCAb0CFAFlO55ngzOaLbXZoPbob2SzUxF2MA0GCSqGSIb3DQEBCwUAMGkx\n" \
    "CzAJBgNVBAYTAkNaMRAwDgYDVQQIDAdDemVjaGlhMQ8wDQYDVQQHDAZQcmFndWUx\n" \
    "EDAOBgNVBAoMB1BydXNhM0QxDTALBgNVBAsMBFRlc3QxFjAUBgNVBAMMDTEwLjI1\n" \
    "LjIzNC4yMDQwHhcNMjEwNTI0MDgwNjI0WhcNMjIwNTI0MDgwNjI0WjBpMQswCQYD\n" \
    "VQQGEwJDWjEQMA4GA1UECAwHQ3plY2hpYTEPMA0GA1UEBwwGUHJhZ3VlMRAwDgYD\n" \
    "VQQKDAdQcnVzYTNEMQ0wCwYDVQQLDARUZXN0MRYwFAYDVQQDDA0xMC4yNS4yMzQu\n" \
    "MjA0MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDbnVYxgBh7H9SV+1c4oy1o\n" \
    "9tWmvHGVEbYEG23SyCEtOvsGqR3b3OW6Hfw9un+2zJceDQnU12UnVWka5c+jp6zS\n" \
    "L1P3Fj0ALqHgzYP5U7zlh0IsIWFZ5UYEatIwck8N1JFDokTewXu2W9gJs5w8v/Xu\n" \
    "lKJ28xs3Gv61E8eBP31SXQIDAQABMA0GCSqGSIb3DQEBCwUAA4GBAD9qfwBp9EmN\n" \
    "Vdt7yXXJh1Nx2FYbTnglNZNd3yAIOKcXqLS8KZ/t7d6WEsyqqDxP8klQLl+cM1cW\n" \
    "mmjPuQ3wwYdjVLPd/GOBkgqsFqiiUD7BgkQv3GoenrHVF/W5Z5ykrw+WlDu1SvYS\n" \
    "o9lvACZckOjD2rpG/uapPdsnL/PnPnLD\n"                                 \
    "-----END CERTIFICATE-----\n"

#define CERT_ECDSA_256                                                   \
    "-----BEGIN CERTIFICATE-----\n"                                      \
    "MIIBgDCCASUCFF7RYrbrfP98TRtB6zu+rN9DQLznMAoGCCqGSM49BAMCMEIxCzAJ\n" \
    "BgNVBAYTAkNaMRAwDgYDVQQIDAdDemVjaGlhMQ8wDQYDVQQHDAZQcmFndWUxEDAO\n" \
    "BgNVBAoMB1Rlc3QgQ0EwHhcNMjEwNTI0MTExNDMyWhcNMjIwNTI0MTExNDMyWjBC\n" \
    "MQswCQYDVQQGEwJDWjEQMA4GA1UECAwHQ3plY2hpYTEPMA0GA1UEBwwGUHJhZ3Vl\n" \
    "MRAwDgYDVQQKDAdUZXN0IENBMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEvXRg\n" \
    "Ph8TbgPbJZQElS8Fx/M2JlYJLBI5d1RefRSufhUOJGSlMyMK6wvjJVvQdqv16ARN\n" \
    "VxDVnjbB2pQmlM9Q4zAKBggqhkjOPQQDAgNJADBGAiEAjZgS7Y54k92X72mNi7Ni\n" \
    "jyOkTNpse228V6ZndK4UBA4CIQDTI+GPSkfcqdHuZ2BSVHGlF2pB15CmArKbbAGr\n" \
    "7y0p1A==\n"                                                         \
    "-----END CERTIFICATE-----\n"

// uncomment any of the below defines to enable corresponding cipher suite
//#define USE_CIPHER_RSA_2048
//#define USE_CIPHER_RSA_1024
#define USE_CIPHER_ECC
#if defined(USE_CIPHER_RSA_2048)
const char my_mbedtls_test_cas_pem[] = CERT_RSA_2048;
#elif defined(USE_CIPHER_RSA_1024)
const char my_mbedtls_test_cas_pem[] = CERT_RSA_1024;
#elif defined(USE_CIPHER_ECC)
const char my_mbedtls_test_cas_pem[] = CERT_ECDSA_256;
#else
    #error "error cipher configuration"
#endif

const size_t my_mbedtls_test_cas_pem_len = sizeof(my_mbedtls_test_cas_pem);

static void my_debug(void *ctx, int level,
    const char *file, int line,
    const char *str) {
    ((void)level);

    _dbg("%s:%04d: %s", file, line, str);
}

static int my_verify(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
    const uint32_t buf_size = 1024;
    char buf[buf_size];
    (void)data;

    _dbg("\nVerifying certificate at depth %d:\n", depth);
    mbedtls_x509_crt_info(buf, buf_size - 1, "  ", crt);
    _dbg("%s", buf);

    if (*flags == 0)
        _dbg("No verification issue for this certificate\n");
    else {
        mbedtls_x509_crt_verify_info(buf, buf_size, "  ! ", *flags);
        _dbg("%s\n", buf);
    }

    return 0;
}

void StartsslTask(void const *argument) {
    osDelay(4000);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    mbedtls_memory_buffer_alloc_init(memory_buf, sizeof(memory_buf));
    int status;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // TRUST CHAIN CONFIGURATION

    mbedtls_x509_crt x509_certificate;
    mbedtls_x509_crt_init(&x509_certificate);

    _dbg("-------------mbedtls starts ----------------------\n");
    mbedtls_x509_crt_init(&x509_certificate);
    status = mbedtls_x509_crt_parse(&x509_certificate, (const unsigned char *)my_mbedtls_test_cas_pem,
        my_mbedtls_test_cas_pem_len);
    if (status < 0) {
        _dbg(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -status);
        goto quite_x509_certificate;
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // ENTROPY/RANDOMNESS SOURCE AND PSEUDORANDOM NUMBER GENERATOR (PRNG) CONFIGURATION

    mbedtls_entropy_context entropy_context;
    mbedtls_entropy_init(&entropy_context);

    mbedtls_ctr_drbg_context drbg_context;
    mbedtls_ctr_drbg_init(&drbg_context);

    if ((status = mbedtls_ctr_drbg_seed(&drbg_context, mbedtls_entropy_func, &entropy_context, NULL, 0)) != 0) {
        _dbg(" mbedtls_ctr_drbg_seed (-0x%X)\n", -status);
        goto quite_entropy;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // TLS CONFIGURATION

    mbedtls_ssl_config ssl_config;
    mbedtls_ssl_config_init(&ssl_config);

    mbedtls_ssl_conf_dbg(&ssl_config, my_debug, NULL);
#ifdef DEBUG
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif
    mbedtls_ssl_conf_verify(&ssl_config, my_verify, NULL);

    if ((status = mbedtls_ssl_config_defaults(&ssl_config,
             MBEDTLS_SSL_IS_CLIENT,
             MBEDTLS_SSL_TRANSPORT_STREAM,
             MBEDTLS_SSL_PRESET_DEFAULT))
        != 0) {
        _dbg(" mbedtls_ssl_config_defaults failed to load default SSL config (-0x%X)\n", -status);
        goto quite_ssl_config;
    }

    // Only use TLS 1.2
#ifndef USE_CIPHER_RSA_1024
    mbedtls_ssl_conf_max_version(&ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_min_version(&ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
#endif
    // set cipher suite to use
#if defined(USE_CIPHER_RSA_1024) || defined(USE_CIPHER_RSA_2048)
    static const int tls_cipher_suites[2] = { MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256, 0 };
#elif defined(USE_CIPHER_ECC)
    static const int tls_cipher_suites[2] = { MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, 0 };
#endif
    mbedtls_ssl_conf_ciphersuites(&ssl_config, tls_cipher_suites);

    // By limiting ourselves to TLS v1.2 and the previous cipher suites, we can compile mbedTLS without the unused ciphers
    // and reduce its size

    // Load CA certificate
    mbedtls_ssl_conf_ca_chain(&ssl_config, &x509_certificate, NULL);
    // Strictly ensure that certificates are signed by the CA
    mbedtls_ssl_conf_authmode(&ssl_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    //    mbedtls_ssl_conf_authmode(&ssl_config, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&ssl_config, mbedtls_ctr_drbg_random, &drbg_context);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // TLS CONTEXT

    mbedtls_ssl_context ssl_context;
    mbedtls_ssl_init(&ssl_context);

    if ((status = mbedtls_ssl_setup(&ssl_context, &ssl_config)) != 0) {
        _dbg(" mbedtls_ssl_setup failed to setup SSL context (-0x%X)\n", -status);
        goto quite_ssl_context;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // ESTABLISH SECURE TLS CONNECTION

    mbedtls_net_context net_context;
    mbedtls_net_init(&net_context);

    mbedtls_ssl_set_bio(&ssl_context, &net_context, mbedtls_net_send, mbedtls_net_recv, NULL);

    if ((status = mbedtls_net_connect(&net_context, SERVER_NAME, SERVER_PORT, MBEDTLS_NET_PROTO_TCP)) != 0) {
        _dbg(" mbedtls_net_connect (-0x%X)\n", -status);
        goto quite_net_context;
    }

    // Verify that that certificate actually belongs to the host
    if ((status = mbedtls_ssl_set_hostname(&ssl_context, SERVER_NAME)) != 0) {
        _dbg(" mbedtls_ssl_set_hostname (-0x%X)\n", -status);
        goto quite_close_context;
    }

    while ((status = mbedtls_ssl_handshake(&ssl_context)) != 0) {
        if (status != MBEDTLS_ERR_SSL_WANT_READ && status != MBEDTLS_ERR_SSL_WANT_WRITE) {
            _dbg(" mbedtls_ssl_handshake (-0x%X)\n", -status);
            goto quite_close_context;
        }
    }

    if ((status = mbedtls_ssl_get_verify_result(&ssl_context)) != 0) {
        _dbg(" mbedtls_ssl_get_verify_result (-0x%X)\n", -status);
        goto quite_close_context;
    }
    _dbg(" mbedtls_ssl_get_verify_result ok\n");

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // EXCHANGE SOME MESSAGES

    const unsigned char write_buffer[] = "Hello world!\n";
    const size_t write_buffer_length = sizeof(write_buffer) - 1; // last byte is the null terminator

    do {
        unsigned char read_buffer[64];
        static const size_t read_buffer_length = sizeof(read_buffer);

        memset(read_buffer, 0, sizeof(read_buffer));

        status = mbedtls_ssl_write(&ssl_context, write_buffer, write_buffer_length);

        if (status == 0) {
            break;
        }

        if (status < 0) {
            switch (status) {
            case MBEDTLS_ERR_SSL_WANT_READ:
            case MBEDTLS_ERR_SSL_WANT_WRITE:
            case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
            case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS: {
                continue;
            }
            default: {
                _dbg(" mbedtls_ssl_write (-0x%X)\n", -status);
                goto quite_close_context;
            }
            }
        }

        status = mbedtls_ssl_read(&ssl_context, read_buffer, read_buffer_length);

        if (status == 0) {
            break;
        }

        if (status < 0) {
            switch (status) {
            case MBEDTLS_ERR_SSL_WANT_READ:
            case MBEDTLS_ERR_SSL_WANT_WRITE:
            case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
            case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS: {
                continue;
            }
            default: {
                _dbg(" mbedtls_ssl_read (-0x%X)\n", -status);
                goto quite_close_context;
            }
            }
        }
    } while (1);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // CLEANUP AND EXIT

quite_close_context:

    if ((status = mbedtls_ssl_close_notify(&ssl_context)) == 0) {
        _dbg("mbedtls_ssl_close_notify sent\n");
    } else {
        _dbg("mbedtls_ssl_close_notify (-0x%X)\n", -status);
    }
quite_net_context:
    mbedtls_net_free(&net_context);

quite_ssl_context:
    mbedtls_ssl_free(&ssl_context);

quite_ssl_config:
    mbedtls_ssl_config_free(&ssl_config);

quite_entropy:
    mbedtls_ctr_drbg_free(&drbg_context);
    mbedtls_entropy_free(&entropy_context);

quite_x509_certificate:
    mbedtls_x509_crt_free(&x509_certificate);
    mbedtls_memory_buffer_alloc_status();
    _dbg("*****************mbedtls ends ----------------------\n");
    while (1) {
        osDelay(100);
    }
}
