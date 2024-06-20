#include "../../lv_examples.h"
#if LV_USE_QRCODE && LV_BUILD_EXAMPLES

/**
 * Create a QR Code
 */
void lv_example_qrcode_1(void )
{
   lv_color_t bg_color = lv_color_white();
    lv_color_t fg_color = lv_palette_darken(0, 0);

    lv_obj_t * qr = lv_qrcode_create(lv_scr_act(), 300, fg_color, bg_color);

    /*Set data*/
    const char * data = "00020101021130450016abaakhppxxx@abaa01090002118870208ABA Bank40390006abaP2P011224D93FFFC17102090002118875204000053038405802KH5911Rithy SOURN6010Phnom Penh6304982F";
    lv_qrcode_update(qr, data, strlen(data));
    
    // lv_obj_set_style_transform_angle(qr,2700, 0);
    lv_obj_align(qr, LV_ALIGN_CENTER, 0, 120);
    /*Add a border with bg_color*/
    lv_obj_set_style_border_color(qr, bg_color, 0);
    lv_obj_set_style_border_width(qr, 5, 0);
}

#endif








