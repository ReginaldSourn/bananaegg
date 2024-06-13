#include "../../lv_examples.h"
#if LV_USE_QRCODE && LV_BUILD_EXAMPLES

/**
 * Create a QR Code
 */
void lv_example_qrcode_1(void)
{
    lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    lv_color_t fg_color = lv_palette_darken(0, 0);

    lv_obj_t * qr = lv_qrcode_create(lv_scr_act(), 300, fg_color, bg_color);

    /*Set data*/
    const char * data = "00020101021229450016abaakhppxxx@abaa01090005096580208ABA Bank40390006abaP2P0112A0207DDBA9540209000509658520400005303840540420.05802KH5914Brilliant PHAL6010Phnom Penh6304C096";
    lv_qrcode_update(qr, data, strlen(data));
    lv_obj_center(qr);

    /*Add a border with bg_color*/
    lv_obj_set_style_border_color(qr, bg_color, 0);
    lv_obj_set_style_border_width(qr, 5, 0);
}

#endif








