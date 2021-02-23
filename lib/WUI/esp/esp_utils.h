#include "esp/esp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \ingroup         ESP
 * \defgroup        ESP_UTILS Utilities
 * \brief           Utilities
 * \{
 */

/**
 * \brief           Assert an input parameter if in valid range
 * \note            Since this is a macro, it may only be used on a functions where return status is of type \ref esp_res_t enumeration
 * \param[in]       msg: message to print to debug if test fails
 * \param[in]       c: Condition to test
 */
#define ESP_ASSERT(msg, c)   do {   \
        if (!(c)) {                     \
            ESP_DEBUGF(ESP_CFG_DBG_ASSERT, "Wrong parameters on file %s and line %d: %s\r\n", __FILE__, (int)__LINE__, msg);\
            return espPARERR;           \
        }                               \
    } while (0)

/**
 * \brief           Align `x` value to specific number of bytes, provided by \ref ESP_CFG_MEM_ALIGNMENT configuration
 * \param[in]       x: Input value to align
 * \return          Input value aligned to specific number of bytes
 * \hideinitializer
 */
#define ESP_MEM_ALIGN(x)                    ((x + (ESP_CFG_MEM_ALIGNMENT - 1)) & ~(ESP_CFG_MEM_ALIGNMENT - 1))

/**
 * \brief           Get minimal value between `x` and `y` inputs
 * \param[in]       x: First input to test
 * \param[in]       y: Second input to test
 * \return          Minimal value between `x` and `y` parameters
 * \hideinitializer
 */
#define ESP_MIN(x, y)                       ((x) < (y) ? (x) : (y))

/**
 * \brief           Get maximal value between `x` and `y` inputs
 * \param[in]       x: First input to test
 * \param[in]       y: Second input to test
 * \return          Maximal value between `x` and `y` parameters
 * \hideinitializer
 */
#define ESP_MAX(x, y)                       ((x) > (y) ? (x) : (y))

/**
 * \brief           Get size of statically declared array
 * \param[in]       x: Input array
 * \return          Number of array elements
 * \hideinitializer
 */
#define ESP_ARRAYSIZE(x)                    (sizeof(x) / sizeof((x)[0]))

/**
 * \brief           Unused argument in a function call
 * \note            Use this on all parameters in a function which are not used to prevent
 *                  compiler warnings complaining about "unused variables"
 * \param[in]       x: Variable which is not used
 * \hideinitializer
 */
#define ESP_UNUSED(x)                       ((void)(x))

/**
 * \brief           Get input value casted to `unsigned 32-bit` value
 * \param[in]       x: Input value
 * \hideinitializer
 */
#define ESP_U32(x)                          ((uint32_t)(x))

/**
 * \brief           Get input value casted to `unsigned 16-bit` value
 * \param[in]       x: Input value
 * \hideinitializer
 */
#define ESP_U16(x)                          ((uint16_t)(x))

/**
 * \brief           Get input value casted to `unsigned 8-bit` value
 * \param[in]       x: Input value
 * \hideinitializer
 */
#define ESP_U8(x)                           ((uint8_t)(x))

/**
 * \brief           Get input value casted to `signed 32-bit` value
 * \param[in]       x: Input value
 * \hideinitializer
 */
#define ESP_I32(x)                          ((int32_t)(x))

/**
 * \brief           Get input value casted to `signed 16-bit` value
 * \param[in]       x: Input value
 * \hideinitializer
 */
#define ESP_I16(x)                          ((int16_t)(x))

/**
 * \brief           Get input value casted to `signed 8-bit` value
 * \param[in]       x: Input value
 * \hideinitializer
 */
#define ESP_I8(x)                           ((int8_t)(x))

/**
 * \brief           Get input value casted to `size_t` value
 * \param[in]       x: Input value
 * \hideinitializer
 */
#define ESP_SZ(x)                           ((size_t)(x))

/**
 * \brief           Convert `unsigned 32-bit` number to string
 * \param[in]       num: Number to convert
 * \param[out]      out: Output variable to save string
 * \return          Pointer to output variable
 * \hideinitializer
 */
#define esp_u32_to_str(num, out)            esp_u32_to_gen_str(ESP_U32(num), (out), 0, 0)

/**
 * \brief           Convert `unsigned 32-bit` number to HEX string
 * \param[in]       num: Number to convert
 * \param[out]      out: Output variable to save string
 * \param[in]       w: Width of output string.
 *                      When number is shorter than width, leading `0` characters will apply
 * \return          Pointer to output variable
 * \hideinitializer
 */
#define esp_u32_to_hex_str(num, out, w)     esp_u32_to_gen_str(ESP_U32(num), (out), 1, (w))

/**
 * \brief           Convert `signed 32-bit` number to string
 * \param[in]       num: Number to convert
 * \param[out]      out: Output variable to save string
 * \return          Pointer to output variable
 * \hideinitializer
 */
#define esp_i32_to_str(num, out)            esp_i32_to_gen_str(ESP_I32(num), (out))

/**
 * \brief           Convert `unsigned 16-bit` number to string
 * \param[in]       num: Number to convert
 * \param[out]      out: Output variable to save string
 * \return          Pointer to output variable
 * \hideinitializer
 */
#define esp_u16_to_str(num, out)            esp_u32_to_gen_str(ESP_U32(ESP_U16(num)), (out), 0, 0)

/**
 * \brief           Convert `unsigned 16-bit` number to HEX string
 * \param[in]       num: Number to convert
 * \param[out]      out: Output variable to save string
 * \param[in]       w: Width of output string.
 *                      When number is shorter than width, leading `0` characters will apply.
 * \return          Pointer to output variable
 * \hideinitializer
 */
#define esp_u16_to_hex_str(num, out, w)     esp_u32_to_gen_str(ESP_U32(ESP_U16(num)), (out), 1, (w))

/**
 * \brief           Convert `signed 16-bit` number to string
 * \param[in]       num: Number to convert
 * \param[out]      out: Output variable to save string
 * \return          Pointer to output variable
 * \hideinitializer
 */
#define esp_i16_to_str(num, out)            esp_i32_to_gen_str(ESP_I32(ESP_I16(num)), (out))

/**
 * \brief           Convert `unsigned 8-bit` number to string
 * \param[in]       num: Number to convert
 * \param[out]      out: Output variable to save string
 * \return          Pointer to output variable
 * \hideinitializer
 */
#define esp_u8_to_str(num, out)             esp_u32_to_gen_str(ESP_U32(ESP_U8(num)), (out), 0, 0)

/**
 * \brief           Convert `unsigned 16-bit` number to HEX string
 * \param[in]       num: Number to convert
 * \param[out]      out: Output variable to save string
 * \param[in]       w: Width of output string.
 *                      When number is shorter than width, leading `0` characters will apply.
 * \return          Pointer to output variable
 * \hideinitializer
 */
#define esp_u8_to_hex_str(num, out, w)      esp_u32_to_gen_str(ESP_U32(ESP_U8(num)), (out), 1, (w))

/**
 * \brief           Convert `signed 8-bit` number to string
 * \param[in]       num: Number to convert
 * \param[out]      out: Output variable to save string
 * \return          Pointer to output variable
 * \hideinitializer
 */
#define esp_i8_to_str(num, out)             esp_i32_to_gen_str(ESP_I32(ESP_I8(num)), (out))

char*       esp_u32_to_gen_str(uint32_t num, char* out, uint8_t is_hex, uint8_t padding);
char*       esp_i32_to_gen_str(int32_t num, char* out);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

