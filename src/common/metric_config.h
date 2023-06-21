///@brief Enums for metric configuration used in config_store()
///@note You can add, but never reorder items.

#include <stdint.h>

/**
 * @brief Allow metrics.
 */
enum class MetricsAllow : uint8_t {
    None = 0, ///< Metrics are not allowed
    One = 1,  ///< Metrics can be enabled only to one selected host
    All = 2,  ///< Metrics can be enabled to any host
};
