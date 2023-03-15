#pragma once

#include <cstdint>
#include <FreeRTOS.h>
#include <event_groups.h>
#include "printers.h"

/**
 * Code in this module handles task/component dependencies
 *
 * Provided tools enable definition of dependency masks, waiting for dependencies being satisfied and marking dependencies as provided.
 */

typedef uint8_t dependency_t;

/// Definition fo different dependencies
enum class ComponentDependencies {
    PUPPIES_READY_IDX = 1,
    RESOURCES_READY_IDX = 2,
    DEFAULT_TASK_READY_IDX = 3,
    USBSERIAL_READY = 4,
    ESP_FLASHED = 5,
    // To be continued...
};

/// Allow shifting
constexpr dependency_t operator<<(const dependency_t &a, const ComponentDependencies &b) {
    return a << static_cast<dependency_t>(b);
};

/// Define dependecy masks
inline constexpr dependency_t PUPPIES_READY = 1 << ComponentDependencies::PUPPIES_READY_IDX;
inline constexpr dependency_t RESOURCES_READY = 1 << ComponentDependencies::RESOURCES_READY_IDX;
inline constexpr dependency_t DEFAULT_TASK_READY = 1 << ComponentDependencies::DEFAULT_TASK_READY_IDX;
inline constexpr dependency_t USBSERIAL_READY = 1 << ComponentDependencies::USBSERIAL_READY;
inline constexpr dependency_t ESP_FLASHED = 1 << ComponentDependencies::ESP_FLASHED;

/// Definitions of dependecies for different tasks/components
inline constexpr dependency_t DEFAULT_TASK_DEPS =
#if PRINTER_TYPE == PRINTER_PRUSA_XL
    PUPPIES_READY |
#endif
    USBSERIAL_READY | 0;
inline constexpr dependency_t PUPPY_TASK_START_DEPS = RESOURCES_READY;
inline constexpr dependency_t PUPPY_TASK_RUN_DEPS = DEFAULT_TASK_READY;

// Needed for inline methods being embedded to different compilation modules
extern EventGroupHandle_t components_ready;

/// Initialize component dependecy resolution
extern void components_init();

/**
 * Return true if dependencies are fulfilled already
 */
inline bool check_dependencies(const dependency_t dependencies) {
    return xEventGroupGetBits(components_ready) & dependencies;
}

/**
 * Wait for dependecies of the task/component
 */
inline void wait_for_dependecies(const dependency_t dependecies) {
    if (dependecies) {
        xEventGroupWaitBits(components_ready, dependecies, pdFALSE, pdTRUE, portMAX_DELAY);
    }
}

/**
 * Mark particular dependecy as satisfied
 */
inline void provide_dependecy(const ComponentDependencies dependecy) {
    xEventGroupSetBits(components_ready, 1 << dependecy);
}
