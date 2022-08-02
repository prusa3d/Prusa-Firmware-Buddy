// resource.c - generated file - do not edit!

#include "config.h"
#include "guitypes.h"
#include "res/cc/font_7x13.c"  //IDR_FNT_SMALL
#include "res/cc/font_11x18.c" //IDR_FNT_NORMAL
                               //#include "res/cc/font_10x18.c" //IDR_FNT_NORMAL
#include "res/cc/font_12x21.c" //IDR_FNT_BIG
#include "res/cc/font_9x15.c"  //IDR_FNT_TERMINAL
#include "res/cc/font_9x16.c"  //IDR_FNT_SPECIAL

#include "res/cc/png_marlin_logo.c" //IDR_PNG_splash_logo_marlin
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "res/cc/png_prusa_mini_splash.c" //IDR_PNG_prusa_printer_splash
    #include "res/cc/png_prusa_mini_logo.c"   //IDR_PNG_prusa_printer_logo
#else
    #error "Unknown PRINTER_TYPE."
#endif // PRINTER_TYPE

#include "res/cc/png_nozzle_16px.c"           //IDR_PNG_nozzle_16px
#include "res/cc/png_heatbed_16px.c"          //IDR_PNG_heatbed_16px
#include "res/cc/png_speed_16px.c"            //IDR_PNG_speed_16px
#include "res/cc/png_spool_16px.c"            //IDR_PNG_spool_16px
#include "res/cc/png_z_axis_16px.c"           //IDR_PNG_z_axis_16px
#include "res/cc/png_home_shape_16px.c"       //IDR_PNG_home_shape_16px
#include "res/cc/png_print_16px.c"            //IDR_PNG_print_16px
#include "res/cc/png_x_axis_16x16.c"          //IDR_PNG_x_axis_16x16,
#include "res/cc/png_y_axis_16x16.c"          //IDR_PNG_y_axis_16x16,
#include "res/cc/png_z_axis_16x16.c"          //IDR_PNG_z_axis_16x16,
#include "res/cc/png_turbine_16x16.c"         //IDR_PNG_turbine_16x16,
#include "res/cc/png_fan_16x16.c"             //IDR_PNG_fan_16x16,
#include "res/cc/png_selftest_16x16.c"        //IDR_PNG_selftest_16x16,
#include "res/cc/png_wizard_16x16.c"          //IDR_PNG_wizard_16x16, // TODO generate with script
#include "res/cc/png_filament_sensor_17x16.c" //IDR_PNG_filament_sensor_17x16

#include "res/cc/png_print_58px.c"       //IDR_PNG_print_58px
#include "res/cc/png_preheat_58px.c"     //IDR_PNG_preheat_58px
#include "res/cc/png_spool_58px.c"       //IDR_PNG_spool_58px
#include "res/cc/png_reprint_48px.c"     //IDR_PNG_reprint_48px
#include "res/cc/png_calibrate_58px.c"   //IDR_PNG_calibrate_58px
#include "res/cc/png_settings_58px.c"    //IDR_PNG_settings_58px
#include "res/cc/png_info_58px.c"        //IDR_PNG_info_58px
#include "res/cc/png_pause_58px.c"       //IDR_PNG_pause_58px
#include "res/cc/png_stop_58px.c"        //IDR_PNG_stop_58px
#include "res/cc/png_resume_48px.c"      //IDR_PNG_resume_48px
#include "res/cc/png_home_58px.c"        //IDR_PNG_home_58px
#include "res/cc/png_back_32px.c"        //IDR_PNG_back_32px
#include "res/cc/png_disconnect_48px.c"  //IDR_PNG_disconnect_48px
#include "res/cc/png_info_48px.c"        //IDR_PNG_info_48px
#include "res/cc/png_error_white_48px.c" //IDR_PNG_error_white_48px
#include "res/cc/png_warning_48px.c"     //IDR_PNG_warning_48px
#include "res/cc/png_question_48px.c"    //IDR_PNG_question_48px

#include "res/cc/png_home_full_16px.c"    //IDR_PNG_home_full_16px
#include "res/cc/png_folder_full_16px.c"  //IDR_PNG_folder_full_16px
#include "res/cc/png_folder_up_16px.c"    //IDR_PNG_folder_up_16px
#include "res/cc/png_folder_shape_16px.c" //IDR_PNG_folder_shape_16px
#include "res/cc/png_folder_open_16px.c"  //IDR_PNG_folder_open_16px

#include "res/cc/png_pepa_64px.c"        //IDR_PNG_pepa_64px
#include "res/cc/png_wifi_large_64x64.c" //IDR_PNG_wifi_large_64x64 // TODO generate with script

#include "res/cc/png_dash_18px.c"            //IDR_PNG_dash_18px
#include "res/cc/png_ok_color_18px.c"        //IDR_PNG_ok_color_18px
#include "res/cc/png_nok_color_18px.c"       //IDR_PNG_nok_color_18px
#include "res/cc/png_spinner1_16px.c"        //IDR_PNG_spinner1_16px
#include "res/cc/png_spinner2_16px.c"        //IDR_PNG_spinner2_16px
#include "res/cc/png_spinner3_16px.c"        //IDR_PNG_spinner3_16px
#include "res/cc/png_spinner4_16px.c"        //IDR_PNG_spinner4_16px
#include "res/cc/png_hourglass_39px.c"       //IDR_PNG_hourglass_39px
#include "res/cc/png_wizard_icon_autohome.c" //IDR_PNG_wizard_icon_autohome
#include "res/cc/png_wizard_icon_search.c"   //IDR_PNG_wizard_icon_search
#include "res/cc/png_wizard_icon_measure.c"  //IDR_PNG_wizard_icon_measure
#include "res/cc/png_hand_154x65.c"          //IDR_PNG_hand_154x65,
#include "res/cc/png_nozzle_34x32.c"         //IDR_PNG_nozzle_34x32,

#include "res/cc/png_pepa_140px.c" //IDR_PNG_pepa_140px

#include "res/cc/png_usb_16px.c"            //IDR_PNG_usb_16px
#include "res/cc/png_lan_16px.c"            //IDR_PNG_lan_16px
#include "res/cc/png_wifi_16px.c"           //IDR_PNG_wifi_16px
#include "res/cc/png_info_16px.c"           //IDR_PNG_info_16px
#include "res/cc/png_error_16px.c"          //IDR_PNG_error_16px
#include "res/cc/png_warning_16px.c"        //IDR_PNG_warning_16px
#include "res/cc/png_question_16px.c"       //IDR_PNG_question_16px
#include "res/cc/png_sheets_profile_16px.c" //IDR_PNG_sheets_profile_16px
#include "res/cc/png_nozzle_shape_48px.c"   //IDR_PNG_nozzle_shape_48px

#include "res/cc/png_arrow_up_16px.c"         //IDR_PNG_arrow_up_16px
#include "res/cc/png_arrow_down_16px.c"       //IDR_PNG_arrow_down_16px
#include "res/cc/png_arrow_left_16px.c"       //IDR_PNG_arrow_left_16px
#include "res/cc/png_arrow_right_16px.c"      //IDR_PNG_arrow_right_16px
#include "res/cc/png_arrow_up_8px.c"          //IDR_PNG_arrow_up_8px
#include "res/cc/png_arrow_down_8px.c"        //IDR_PNG_arrow_down_8px
#include "res/cc/png_arrow_up_orange_8px.c"   //IDR_PNG_arrow_up_orange_8px
#include "res/cc/png_arrow_down_orange_8px.c" //IDR_PNG_arrow_down_orange_8px

#include "res/cc/png_serial_printing.c"     //IDR_PNG_serial_printing
#include "res/cc/png_switch_off_36px.c"     //IDR_PNG_switch_off_36px
#include "res/cc/png_switch_on_36px.c"      //IDR_PNG_switch_on_36px
#include "res/cc/png_hand_qr.c"             //IDR_PNG_hand_qr
#include "res/cc/png_exposure_times_48px.c" //IDR_PNG_exposure_times_48px
#include "res/cc/png_usb_error.c"           //IDR_PNG_usb_error
#include "res/cc/png_fan_error.c"           //IDR_PNG_fan_error
#include "res/cc/png_turn_knob.c"           //IDR_PNG_turn_knob

#include "res/cc/png_flag_cs.c"
#include "res/cc/png_flag_de.c"
#include "res/cc/png_flag_en.c"
#include "res/cc/png_flag_es.c"
#include "res/cc/png_flag_fr.c"
#include "res/cc/png_flag_it.c"
#include "res/cc/png_flag_pl.c"

RESOURCE_TABLE_BEGIN
RESOURCE_ENTRY_NUL() //IDR_NULL
//fonts
RESOURCE_ENTRY_FNT(font_7x13)  //IDR_FNT_SMALL
RESOURCE_ENTRY_FNT(font_11x18) //IDR_FNT_NORMAL
//RESOURCE_ENTRY_FNT(font_10x18) //IDR_FNT_NORMAL
RESOURCE_ENTRY_FNT(font_12x21) //IDR_FNT_BIG
RESOURCE_ENTRY_FNT(font_9x15)  //IDR_FNT_TERMINAL
RESOURCE_ENTRY_FNT(font_9x16)  //IDR_FNT_SPECIAL

//pngs
RESOURCE_ENTRY_PNG(png_marlin_logo)          //IDR_PNG_malin_logo
RESOURCE_ENTRY_PNG(png_prusa_printer_splash) //IDR_PNG_prusa_printer_splash
RESOURCE_ENTRY_PNG(png_prusa_printer_logo)   //IDR_PNG_prusa_printer_logo

RESOURCE_ENTRY_PNG(png_nozzle_16px)
RESOURCE_ENTRY_PNG(png_heatbed_16px)
RESOURCE_ENTRY_PNG(png_speed_16px)
RESOURCE_ENTRY_PNG(png_spool_16px)
RESOURCE_ENTRY_PNG(png_z_axis_16px)
RESOURCE_ENTRY_PNG(png_home_shape_16px)
RESOURCE_ENTRY_PNG(png_print_16px)
RESOURCE_ENTRY_PNG(png_x_axis_16x16)
RESOURCE_ENTRY_PNG(png_y_axis_16x16)
RESOURCE_ENTRY_PNG(png_z_axis_16x16)
RESOURCE_ENTRY_PNG(png_turbine_16x16)
RESOURCE_ENTRY_PNG(png_fan_16x16)
RESOURCE_ENTRY_PNG(png_selftest_16x16)
RESOURCE_ENTRY_PNG(png_wizard_16x16)
RESOURCE_ENTRY_PNG(png_filament_sensor_17x16)

RESOURCE_ENTRY_PNG(png_print_58px)
RESOURCE_ENTRY_PNG(png_preheat_58px)
RESOURCE_ENTRY_PNG(png_spool_58px)
RESOURCE_ENTRY_PNG(png_reprint_48px)
RESOURCE_ENTRY_PNG(png_calibrate_58px)
RESOURCE_ENTRY_PNG(png_settings_58px)
RESOURCE_ENTRY_PNG(png_info_58px)
RESOURCE_ENTRY_PNG(png_pause_58px)
RESOURCE_ENTRY_PNG(png_stop_58px)
RESOURCE_ENTRY_PNG(png_resume_48px)
RESOURCE_ENTRY_PNG(png_home_58px)
RESOURCE_ENTRY_PNG(png_info_48px)
RESOURCE_ENTRY_PNG(png_error_white_48px)
RESOURCE_ENTRY_PNG(png_warning_48px)
RESOURCE_ENTRY_PNG(png_question_48px)

RESOURCE_ENTRY_PNG(png_home_full_16px)
RESOURCE_ENTRY_PNG(png_folder_full_16px)
RESOURCE_ENTRY_PNG(png_folder_up_16px)
RESOURCE_ENTRY_PNG(png_folder_shape_16px)
RESOURCE_ENTRY_PNG(png_folder_open_16px)

RESOURCE_ENTRY_PNG(png_pepa_64px)
RESOURCE_ENTRY_PNG(png_wifi_large_64x64)

RESOURCE_ENTRY_PNG(png_dash_18px)
RESOURCE_ENTRY_PNG(png_ok_color_18px)
RESOURCE_ENTRY_PNG(png_nok_color_18px)
RESOURCE_ENTRY_PNG(png_spinner1_16px)
RESOURCE_ENTRY_PNG(png_spinner2_16px)
RESOURCE_ENTRY_PNG(png_spinner3_16px)
RESOURCE_ENTRY_PNG(png_spinner4_16px)
RESOURCE_ENTRY_PNG(png_hourglass_39px)
RESOURCE_ENTRY_PNG(png_wizard_icon_autohome)
RESOURCE_ENTRY_PNG(png_wizard_icon_search)
RESOURCE_ENTRY_PNG(png_wizard_icon_measure)
RESOURCE_ENTRY_PNG(png_hand_154x65)
RESOURCE_ENTRY_PNG(png_nozzle_34x32)

RESOURCE_ENTRY_PNG(png_pepa_140px)

RESOURCE_ENTRY_PNG(png_usb_16px)
RESOURCE_ENTRY_PNG(png_lan_16px)
RESOURCE_ENTRY_PNG(png_wifi_16px)
RESOURCE_ENTRY_PNG(png_info_16px)
RESOURCE_ENTRY_PNG(png_error_16px)
RESOURCE_ENTRY_PNG(png_warning_16px)
RESOURCE_ENTRY_PNG(png_question_16px)
RESOURCE_ENTRY_PNG(png_sheets_profile_16px)
RESOURCE_ENTRY_PNG(png_nozzle_shape_48px)

RESOURCE_ENTRY_PNG(png_arrow_up_16px)
RESOURCE_ENTRY_PNG(png_arrow_down_16px)
RESOURCE_ENTRY_PNG(png_arrow_left_16px)
RESOURCE_ENTRY_PNG(png_arrow_right_16px)
RESOURCE_ENTRY_PNG(png_arrow_up_8px)
RESOURCE_ENTRY_PNG(png_arrow_down_8px)
RESOURCE_ENTRY_PNG(png_arrow_up_orange_8px)
RESOURCE_ENTRY_PNG(png_arrow_down_orange_8px)

RESOURCE_ENTRY_PNG(png_back_32px)
RESOURCE_ENTRY_PNG(png_serial_printing)
RESOURCE_ENTRY_PNG(png_disconnect_48px)
RESOURCE_ENTRY_PNG(png_switch_off_36px)
RESOURCE_ENTRY_PNG(png_switch_on_36px)
RESOURCE_ENTRY_PNG(png_hand_qr)
RESOURCE_ENTRY_PNG(png_exposure_times_48px)
RESOURCE_ENTRY_PNG(png_usb_error)
RESOURCE_ENTRY_PNG(png_fan_error)
RESOURCE_ENTRY_PNG(png_turn_knob)

RESOURCE_ENTRY_PNG(png_flag_cs)
RESOURCE_ENTRY_PNG(png_flag_de)
RESOURCE_ENTRY_PNG(png_flag_en)
RESOURCE_ENTRY_PNG(png_flag_es)
RESOURCE_ENTRY_PNG(png_flag_fr)
RESOURCE_ENTRY_PNG(png_flag_it)
RESOURCE_ENTRY_PNG(png_flag_pl)
RESOURCE_TABLE_END
