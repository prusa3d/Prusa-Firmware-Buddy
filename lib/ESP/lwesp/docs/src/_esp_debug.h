/**
 * \addtogroup      ESP_DEBUG
 * \{
 *
 * Library supports different debugging options (similar to LwIP stack).
 *
 * \par				Debugging output function
 *
 * By default configuration, `printf` function is set as debug output function.
 * If user wants to change behavior, check \ref ESP_CFG_DBG_OUT function in configuration.
 *
 * \par				Enable global debugging
 *
 * Global debug support is disabled by default with configuration
 * parameter \ref ESP_CFG_DBG and is set to \ref ESP_DBG_OFF.
 *
 * To enable global debug support, set configuration to \ref ESP_DBG_ON.
 *
 * \par             Enable type debugging
 *
 * Different debug types may be applied, such as tracing or state support.
 *
 * Tracing debug may be used to print program flow messages in comparison
 * with state which may be used to debug debug states such as state machines.
 *
 * By default all types are disabled and must be enabled by modifying
 * \ref ESP_CFG_DBG_TYPES_ON configuration parameter.
 *
 * \par             Debug level
 *
 * All debug levels are enabled by default, but this may be changes
 * to print only specific debug types like warning or danger messages.
 *
 * \par             Example code
 *
 * Modifications of config file to enable basic debug:
 *
 * \include         _example_debug_config.h
 *
 * Use configuration in your program:
 *
 * \include         _example_debug.c
 * 
 * \}
 */

/**
 * \addtogroup      ESP_CONFIG_DBG
 * \{
 *
 * Check \ref ESP_DEBUG section for more information
 *
 * \}
 */