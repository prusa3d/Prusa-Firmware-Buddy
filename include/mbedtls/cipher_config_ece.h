#pragma once

/* System support */
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_HAVE_TIME

/* mbed TLS feature support */
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_SSL_PROTO_TLS1_2

// Needed for certificate expiration checks.
#define MBEDTLS_HAVE_TIME_DATE
// Don't disable important security checks.
#define MBEDTLS_X509_CHECK_KEY_USAGE
#define MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE

/* mbed TLS modules */
#define MBEDTLS_AES_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_NO_PLATFORM_ENTROPY
// We don't need full 20 sources pre-configured and we want the structure to
// fit into 512B.
#define MBEDTLS_ENTROPY_MAX_SOURCES 19
#define MBEDTLS_MD_C
#define MBEDTLS_OID_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_MD5_C
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_USE_C

/* for enabling SNI extension*/
#define MBEDTLS_SSL_SERVER_NAME_INDICATION

#define MBEDTLS_GCM_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_C

#define MBEDTLS_SSL_CIPHERSUITES MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#define MBEDTLS_AES_ROM_TABLES
#define MBEDTLS_MPI_MAX_SIZE          512
#define MBEDTLS_MPI_WINDOW_SIZE       1
#define MBEDTLS_ECP_WINDOW_SIZE       2
#define MBEDTLS_ECP_FIXED_POINT_OPTIM 0
#define MBEDTLS_SSL_MAX_CONTENT_LEN   1024

#define MBEDTLS_SSL_OUT_CONTENT_LEN 512
#define MBEDTLS_SSL_IN_CONTENT_LEN  1024
// Tell the server we don't want big fragments (relates to MBEDTLS_SSL_OUT_CONTENT_LEN)
#define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
// Prefer smaller code over fast computations (our CPU is idle most of the time anyway).
#define MBEDTLS_AES_FEWER_TABLES
#define MBEDTLS_SHA256_SMALLER

// For symmetric connect transfer encrypiton
#define MBEDTLS_CIPHER_MODE_CTR

#define MBEDTLS_BASE64_C

#include "mbedtls/check_config.h"
