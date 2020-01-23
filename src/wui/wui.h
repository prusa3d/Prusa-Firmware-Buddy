/*
 * wui.h
 * \brief main interface functions for Web User Interface (WUI) thread
 *
 *  Created on: Dec 12, 2019
 *      Author: joshy
 */

#ifndef SRC_WUI_WUI_H_
#define SRC_WUI_WUI_H_

#define BUDDY_WEB_STACK_SIZE	512
#include <marlin_vars.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!****************************************************************************
* \brief initalize necessary things for Web UI interface
*
* \param
*
* \return
*
*****************************************************************************/
void init_wui(void);

/*!****************************************************************************
* \brief Webserver thread function
*
*****************************************************************************/
void StartWebServerTask(void const *argument);

/*!****************************************************************************
* \brief update the marlin variable for the current thread access
*
* \param    void
*
* \return	void
*
*****************************************************************************/
void marlin_var_update();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* SRC_WUI_WUI_H_ */

