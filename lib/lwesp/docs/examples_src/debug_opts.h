/* Modifications of lwesp_opts.h file for configuration */

/* Enable global debug */
#define LWESP_CFG_DBG               LWESP_DBG_ON

/*
 * Enable debug types.
 * Application may use bitwise OR | to use multiple types:
 *    LWESP_DBG_TYPE_TRACE | LWESP_DBG_TYPE_STATE
 */
#define LWESP_CFG_DBG_TYPES_ON      LWESP_DBG_TYPE_TRACE

/* Enable debug on custom module */
#define MY_DBG_MODULE               LWESP_DBG_ON
