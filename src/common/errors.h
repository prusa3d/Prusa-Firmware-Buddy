// errors.h
#ifndef _ERRORS_H
#define _ERRORS_H

// ***** CATEGORY definitions
typedef enum {
    ERR_CAT_UNDEF = 0,
    ERR_CAT_MECHANICAL = 1,
    ERR_CAT_THERMAL = 2,
    ERR_CAT_ELECTRO = 3,
    ERR_CAT_CONNECT = 4,
    ERR_CAT_SYSTEM = 5,
    ERR_CAT_OTHER = 9
} err_category_t;

// ***** SUBCATEGORY definitions
typedef enum {
    ERR_SUBCAT_UNDEF = 0,
    // ##### category MECHANICAL
    ERR_CAT_MECHANICAL_SUBCAT_MOTOR = 1,
    ERR_CAT_MECHANICAL_SUBCAT_AXIS = 2,
    ERR_CAT_MECHANICAL_SUBCAT_OTHER = 9,

    // ##### category THERMAL
    ERR_CAT_THERMAL_SUBCAT_THERMISTOR = 1,
    ERR_CAT_THERMAL_SUBCAT_BED = 2,
    ERR_CAT_THERMAL_SUBCAT_HOTEND = 3,
    ERR_CAT_THERMAL_SUBCAT_OTHER = 9,

    // ##### category ELECTRO
    ERR_CAT_ELECTRO_SUBCAT_PINDA = 1,
    ERR_CAT_ELECTRO_SUBCAT_FSENSOR = 2,
    ERR_CAT_ELECTRO_SUBCAT_OTHER = 9,
} err_subcategory_t;

// ***** ITEM definitions
typedef enum {
    ERR_ITEM_UNDEF = 0,
    // ##### category THERMAL

    // ***** subcategory BED
    ERR_CAT_THERMAL_SUBCAT_BED_MINTEMP = 1,
    ERR_CAT_THERMAL_SUBCAT_BED_MAXTEMP = 2,
    ERR_CAT_THERMAL_SUBCAT_BED_RUNAWAY = 3,
    ERR_CAT_THERMAL_SUBCAT_BED_HFAIL = 4,
    ERR_CAT_THERMAL_SUBCAT_BED_OTHER = 9,

    // ***** subcategory HOTEND
    ERR_CAT_THERMAL_SUBCAT_HOTEND_MINTEMP = 1,
    ERR_CAT_THERMAL_SUBCAT_HOTEND_MAXTEMP = 2,
    ERR_CAT_THERMAL_SUBCAT_HOTEND_RUNAWAY = 3,
    ERR_CAT_THERMAL_SUBCAT_HOTEND_HFAIL = 4,
    ERR_CAT_THERMAL_SUBCAT_HOTEND_OTHER = 9,

    // ##### category ELECTRO

    // ***** subcategory PINDA
    ERR_CAT_ELECTRO_SUBCAT_PINDA_WIRING = 1,
    ERR_CAT_ELECTRO_SUBCAT_PINDA_OTHER = 9,

    // ***** subcategory FSENSOR
    ERR_CAT_ELECTRO_SUBCAT_FSENSOR_WIRING = 1,
    ERR_CAT_ELECTRO_SUBCAT_FSENSOR_OTHER = 9,
} err_item_t;

typedef enum {
    ART_NONE = 0,
    ART_OTHER = 999,
    ART122 = 122,
    ART124 = 124,
} err_art_list_t;

typedef struct {
    err_category_t cat_num;
    err_subcategory_t subcat_num;
    err_item_t item_num;
    //    int32_t cat_num;
    //    int32_t subcat_num;
    //    int32_t item_num;
} err_num_t;

typedef struct {
    err_num_t err_num;
    const char *err_sign;
    const char *err_title;
    const char *err_text;
    err_art_list_t err_article;
} err_t;

extern void set_actual_error(err_num_t err_item);
extern void set_actual_error(err_category_t err_category, err_subcategory_t err_subcategory, err_item_t err_item);
extern const err_t *get_actual_error(void);

#endif // _ERRORS_H
