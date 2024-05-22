#pragma once
/**
 * @file network_gui_tools.hpp
 * @author Michal Rudolf
 * @brief toolshed for network-gui related functions
 * @date 2022-1-21
 */

#include "netif_settings.h"

/*!*****************************************************************************************
 * \brief Parses all vital eth information in destination string according to screen format !one by one!
 *
 * \param [out] destination - null-terminated string
 * \param [in] dest_len - destination string length
 * \param [in] config - storage for ethernet configurations
 * \param [in] eth_var - which eth var we want to print !one by one!
 *******************************************************************************************/
void stringify_address_for_screen(char *dest, size_t dest_len, const lan_t config, uint32_t eth_var);
