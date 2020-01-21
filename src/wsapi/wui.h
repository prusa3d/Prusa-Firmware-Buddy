/*
 * wui.h
 * \brief main interface functions for Web UI interface
 *
 *  Created on: Dec 12, 2019
 *      Author: joshy
 */

#ifndef SRC_WUI_WUI_H_
#define SRC_WUI_WUI_H_

#include <marlin_vars.h>
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
* \brief update the marlin variable for the current thread access
*
* \param    void
*
* \return	void
*
*****************************************************************************/
void marlin_var_update();

#endif /* SRC_WUI_WUI_H_ */

