cmake_minimum_required(VERSION 3.12)

project(mbedtls VERSION 2.26.0)

option(USE_STATIC_MBEDTLS_LIBRARY "Build mbed TLS static library." ON)
option(USE_SHARED_MBEDTLS_LIBRARY "Build mbed TLS shared library." OFF)

if(NOT DEFINED MBEDTLS_DIR)
  set(MBEDTLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mbedtls)
endif()

set(MBEDTLS_LIB_DIR ${MBEDTLS_DIR}/library)

set(src_crypto
    ${MBEDTLS_LIB_DIR}/aes.c
    ${MBEDTLS_LIB_DIR}/aesni.c
    ${MBEDTLS_LIB_DIR}/arc4.c
    ${MBEDTLS_LIB_DIR}/aria.c
    ${MBEDTLS_LIB_DIR}/asn1parse.c
    ${MBEDTLS_LIB_DIR}/asn1write.c
    ${MBEDTLS_LIB_DIR}/base64.c
    ${MBEDTLS_LIB_DIR}/bignum.c
    ${MBEDTLS_LIB_DIR}/blowfish.c
    ${MBEDTLS_LIB_DIR}/camellia.c
    ${MBEDTLS_LIB_DIR}/ccm.c
    ${MBEDTLS_LIB_DIR}/chacha20.c
    ${MBEDTLS_LIB_DIR}/chachapoly.c
    ${MBEDTLS_LIB_DIR}/cipher.c
    ${MBEDTLS_LIB_DIR}/cipher_wrap.c
    ${MBEDTLS_LIB_DIR}/cmac.c
    ${MBEDTLS_LIB_DIR}/ctr_drbg.c
    ${MBEDTLS_LIB_DIR}/des.c
    ${MBEDTLS_LIB_DIR}/dhm.c
    ${MBEDTLS_LIB_DIR}/ecdh.c
    ${MBEDTLS_LIB_DIR}/ecdsa.c
    ${MBEDTLS_LIB_DIR}/ecjpake.c
    ${MBEDTLS_LIB_DIR}/ecp.c
    ${MBEDTLS_LIB_DIR}/ecp_curves.c
    ${MBEDTLS_LIB_DIR}/entropy.c
    ${MBEDTLS_LIB_DIR}/entropy_poll.c
    ${MBEDTLS_LIB_DIR}/error.c
    ${MBEDTLS_LIB_DIR}/gcm.c
    ${MBEDTLS_LIB_DIR}/havege.c
    ${MBEDTLS_LIB_DIR}/hkdf.c
    ${MBEDTLS_LIB_DIR}/hmac_drbg.c
    ${MBEDTLS_LIB_DIR}/md.c
    ${MBEDTLS_LIB_DIR}/md2.c
    ${MBEDTLS_LIB_DIR}/md4.c
    ${MBEDTLS_LIB_DIR}/md5.c
    ${MBEDTLS_LIB_DIR}/memory_buffer_alloc.c
    ${MBEDTLS_LIB_DIR}/nist_kw.c
    ${MBEDTLS_LIB_DIR}/oid.c
    ${MBEDTLS_LIB_DIR}/padlock.c
    ${MBEDTLS_LIB_DIR}/pem.c
    ${MBEDTLS_LIB_DIR}/pk.c
    ${MBEDTLS_LIB_DIR}/pk_wrap.c
    ${MBEDTLS_LIB_DIR}/pkcs12.c
    ${MBEDTLS_LIB_DIR}/pkcs5.c
    ${MBEDTLS_LIB_DIR}/pkparse.c
    ${MBEDTLS_LIB_DIR}/pkwrite.c
    ${MBEDTLS_LIB_DIR}/platform.c
    ${MBEDTLS_LIB_DIR}/platform_util.c
    ${MBEDTLS_LIB_DIR}/poly1305.c
    ${MBEDTLS_LIB_DIR}/psa_crypto.c
    ${MBEDTLS_LIB_DIR}/psa_crypto_client.c
    ${MBEDTLS_LIB_DIR}/psa_crypto_driver_wrappers.c
    ${MBEDTLS_LIB_DIR}/psa_crypto_ecp.c
    ${MBEDTLS_LIB_DIR}/psa_crypto_rsa.c
    ${MBEDTLS_LIB_DIR}/psa_crypto_se.c
    ${MBEDTLS_LIB_DIR}/psa_crypto_slot_management.c
    ${MBEDTLS_LIB_DIR}/psa_crypto_storage.c
    ${MBEDTLS_LIB_DIR}/psa_its_file.c
    ${MBEDTLS_LIB_DIR}/ripemd160.c
    ${MBEDTLS_LIB_DIR}/rsa.c
    ${MBEDTLS_LIB_DIR}/rsa_internal.c
    ${MBEDTLS_LIB_DIR}/sha1.c
    ${MBEDTLS_LIB_DIR}/sha256.c
    ${MBEDTLS_LIB_DIR}/sha512.c
    ${MBEDTLS_LIB_DIR}/threading.c
    ${MBEDTLS_LIB_DIR}/timing.c
    ${MBEDTLS_LIB_DIR}/version.c
    ${MBEDTLS_LIB_DIR}/version_features.c
    ${MBEDTLS_LIB_DIR}/xtea.c
    )

list(APPEND src_crypto ${thirdparty_src})

set(src_x509
    ${MBEDTLS_LIB_DIR}/certs.c
    ${MBEDTLS_LIB_DIR}/pkcs11.c
    ${MBEDTLS_LIB_DIR}/x509.c
    ${MBEDTLS_LIB_DIR}/x509_create.c
    ${MBEDTLS_LIB_DIR}/x509_crl.c
    ${MBEDTLS_LIB_DIR}/x509_crt.c
    ${MBEDTLS_LIB_DIR}/x509_csr.c
    ${MBEDTLS_LIB_DIR}/x509write_crt.c
    ${MBEDTLS_LIB_DIR}/x509write_csr.c
    )

set(src_tls
    ${MBEDTLS_LIB_DIR}/debug.c
    ${CMAKE_CURRENT_SOURCE_DIR}/prusa/net_sockets.c
    ${MBEDTLS_LIB_DIR}/ssl_cache.c
    ${MBEDTLS_LIB_DIR}/ssl_ciphersuites.c
    ${MBEDTLS_LIB_DIR}/ssl_cli.c
    ${MBEDTLS_LIB_DIR}/ssl_cookie.c
    ${MBEDTLS_LIB_DIR}/ssl_msg.c
    ${MBEDTLS_LIB_DIR}/ssl_srv.c
    ${MBEDTLS_LIB_DIR}/ssl_ticket.c
    ${MBEDTLS_LIB_DIR}/ssl_tls.c
    ${MBEDTLS_LIB_DIR}/ssl_tls13_keys.c
    )

set(mbedtls_target "${MBEDTLS_TARGET_PREFIX}mbedtls")
set(mbedx509_target "${MBEDTLS_TARGET_PREFIX}mbedx509")
set(mbedcrypto_target "${MBEDTLS_TARGET_PREFIX}mbedcrypto")

set(mbedtls_target
    ${mbedtls_target}
    PARENT_SCOPE
    )
set(mbedx509_target
    ${mbedx509_target}
    PARENT_SCOPE
    )
set(mbedcrypto_target
    ${mbedcrypto_target}
    PARENT_SCOPE
    )

set(mbedtls_static_target ${mbedtls_target})
set(mbedx509_static_target ${mbedx509_target})
set(mbedcrypto_static_target ${mbedcrypto_target})

add_library(${mbedcrypto_static_target} STATIC ${src_crypto})
set_target_properties(${mbedcrypto_static_target} PROPERTIES OUTPUT_NAME mbedcrypto)
target_link_libraries(${mbedcrypto_static_target} PUBLIC ${libs})

add_library(${mbedx509_static_target} STATIC ${src_x509})
set_target_properties(${mbedx509_static_target} PROPERTIES OUTPUT_NAME mbedx509)
target_link_libraries(${mbedx509_static_target} PUBLIC ${libs} ${mbedcrypto_static_target})

add_library(${mbedtls_static_target} STATIC ${src_tls})
set_target_properties(${mbedtls_static_target} PROPERTIES OUTPUT_NAME mbedtls)
target_link_libraries(${mbedtls_static_target} PUBLIC ${libs} ${mbedx509_static_target})

set(target_libraries ${mbedcrypto_target} ${mbedx509_target} ${mbedtls_target})

foreach(target IN LISTS target_libraries)
  target_include_directories(
    ${target}
    PUBLIC ${MBEDTLS_DIR}/include/
    PRIVATE ${MBEDTLS_LIB_DIR}/
    PRIVATE ${CMAKE_SOURCE_DIR}/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/src/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/LwIP/system
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/FreeRTOS/Source/include
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
    PRIVATE ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
    PRIVATE ${CMAKE_SOURCE_DIR}/include/freertos
    )
  target_compile_definitions(${target} PRIVATE "MBEDTLS_CONFIG_FILE=<mbedtls_config.h>")
endforeach(target)
