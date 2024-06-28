/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"

#include "examples/lv_examples.h"

#include "driver/i2c.h"
#include "esp_lcd_touch_gt911.h"

#include <string.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_spiffs.h"
#include "sdmmc_cmd.h"
#include "wear_levelling.h"
#include "nvs_flash.h"
#include "esp_partition.h"
#include "ff.h"
#include <dirent.h>
// #include "duck.h"
// #include "khqr.h"
// #include "dollar.h"
#define EXAMPLE_MAX_CHAR_SIZE    64

#define MOUNT_POINT "/sdcard"

// Pin assignments can be set in menuconfig, see "SD SPI Example Configuration" menu.
// You can also change the pin assignments here by changing the following 4 lines.
#define PIN_NUM_MISO  11
#define PIN_NUM_MOSI  13
#define PIN_NUM_CLK   12
#define PIN_NUM_CS    -1

#define I2C_MASTER_SCL_IO           GPIO_NUM_41       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           GPIO_NUM_42          /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0       /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000
#define STORAGE_PARTITION_LABEL "storage"
#define LV_USE_QRCODE
static const char *TAG_LCD = "LCD";
static const char *TAG_MEM = "FlashLog";
uint8_t sd_flag = 0;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (9 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  -1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_BK_LIGHT       GPIO_NUM_1
#define EXAMPLE_PIN_NUM_HSYNC          GPIO_NUM_4
#define EXAMPLE_PIN_NUM_VSYNC          GPIO_NUM_2
#define EXAMPLE_PIN_NUM_DE             GPIO_NUM_5
#define EXAMPLE_PIN_NUM_PCLK           GPIO_NUM_6
#define EXAMPLE_PIN_NUM_DATA0          GPIO_NUM_7 // B3
#define EXAMPLE_PIN_NUM_DATA1          GPIO_NUM_15 // B4
#define EXAMPLE_PIN_NUM_DATA2          GPIO_NUM_16 // B5
#define EXAMPLE_PIN_NUM_DATA3          GPIO_NUM_8 // B6
#define EXAMPLE_PIN_NUM_DATA4          GPIO_NUM_3 // B7
#define EXAMPLE_PIN_NUM_DATA5          GPIO_NUM_46 // G2
#define EXAMPLE_PIN_NUM_DATA6          GPIO_NUM_9 // G3
#define EXAMPLE_PIN_NUM_DATA7          GPIO_NUM_10 // G4
#define EXAMPLE_PIN_NUM_DATA8          GPIO_NUM_11 // G5
#define EXAMPLE_PIN_NUM_DATA9          GPIO_NUM_12 // G6
#define EXAMPLE_PIN_NUM_DATA10         GPIO_NUM_13 // G7
#define EXAMPLE_PIN_NUM_DATA11         GPIO_NUM_14  // R3
#define EXAMPLE_PIN_NUM_DATA12         GPIO_NUM_21  // R4
#define EXAMPLE_PIN_NUM_DATA13         GPIO_NUM_47 // R5
#define EXAMPLE_PIN_NUM_DATA14         GPIO_NUM_48 // R6
#define EXAMPLE_PIN_NUM_DATA15         GPIO_NUM_45 // R7
#define EXAMPLE_PIN_NUM_DISP_EN        -1
#define ESP_VFS_PATH_MAX             10
// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_V_RES              856
#define EXAMPLE_LCD_H_RES              480

#if CONFIG_EXAMPLE_DOUBLE_FB
#define EXAMPLE_LCD_NUM_FB             2
#else
#define EXAMPLE_LCD_NUM_FB             1
#endif // CONFIG_EXAMPLE_DOUBLE_FB

#define EXAMPLE_LVGL_TICK_PERIOD_MS    2

// we use two semaphores to sync the VSYNC event and the LVGL task, to avoid potential tearing effect
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
SemaphoreHandle_t sem_vsync_end;
SemaphoreHandle_t sem_gui_ready;
#endif
static int32_t scene_act = -1;
// static lv_obj_t * scene_bg;
static lv_style_t style_common;
static lv_obj_t *scr;

// user declarations 
//


    // LV  OBJECT 
    LV_IMG_DECLARE(khqr);
    LV_IMG_DECLARE(dollar);
    LV_IMG_DECLARE(aba_pay);
    LV_IMG_DECLARE(acleda_white);
    // LV_IMG_DECLARE(sathapana);
    LV_IMG_DECLARE(canadia)
    LV_IMG_DECLARE(riel);
    // Qrcode object 
     static lv_obj_t * qr;
    static lv_obj_t * qr_hide;
    // rect app
    
    static lv_obj_t * rect_home_visible;
    static lv_obj_t * rect_home_hide; 
    static lv_obj_t * rect_list;
    static lv_obj_t * rect_wifi;
    static lv_obj_t * rect_setting;
    static lv_obj_t * rect_menu;

    // QR Devices
    static lv_obj_t * khqr_bg;
    static lv_obj_t * currency_img;
    static lv_obj_t * bank_img_disp;
    static lv_obj_t * t_name;
    static lv_obj_t * t_price;
    // hiding object ; 
    static lv_obj_t * khqr_bg_hide;
    static lv_obj_t * t_name_hide;
    static lv_obj_t * t_price_hide;
    static lv_obj_t * currency_img_hide;
    static lv_obj_t * bank_img_hide;
    // Button menu
    
    static lv_obj_t * b_listbank;
    static lv_obj_t * b_qrwifi;
    static lv_obj_t * b_foodmenu;
    static lv_obj_t * b_setting;
    static lv_obj_t * b_home;
    
    // label menu
    static lv_obj_t * label_lb;
    static lv_obj_t * label_qrwifi;
    static lv_obj_t * label_foodmenu;
    static lv_obj_t * label_setting;
    static lv_obj_t * label_home;

// Style 
    static lv_style_t  style_btn_menu;         // Style for the button
    static lv_style_t  style_btn_menu_tgl;    // Style for the toggled state
    static lv_style_t  style_label_normal;   // Normal state
    static lv_style_t  style_label_checked;  // Checked state
    static lv_color_t qr_bg_color;
    static lv_color_t qr_fg_color;

    // Label 
    static lv_style_t style_t_name;

   
    static lv_style_t style_t_price;
    
//

static lv_img_dsc_t * list_bank[] = {&acleda_white,&aba_pay,&canadia};
static  char* rielqr[] = {
    // dummy riel qr
    "00020101021129360009khqr@aclb01090125202660206ACLEDA3920001185519766794010145204200053031165802KH5911SOURN RITHY6010PHNOM PENH6213020901252026663049032",
    "00020101021129450016abaakhppxxx@abaa01090096294450208ABA Bank40390006abaP2P011224D93FFFC17102090096294455204000053031165802KH5911Rithy SOURN6010Phnom Penh6304DB16",
    "00020101021129450016cadikhppxxx@cadi011306800000334140204cadi5204599953031165802KH5914PHAL BRILLIANT6010Phnom Penh9917001317170525193466304C617",
    
};
static char* dollarqr[] = {
     // dummy dollar qr
    "00020101021229360009khqr@aclb01090125202660206ACLEDA392000118551976679401014520420005303840540510.005802KH5911SOURN RITHY6010PHNOM PENH6213020901252026663046A5E", 
    "00020101021229450016abaakhppxxx@abaa01090002118870208ABA Bank40390006abaP2P011224D93FFFC1710209000211887520400005303840540410.05802KH5911Rithy SOURN6010Phnom Penh6304EA02",
    "00020101021129450016cadikhppxxx@cadi011306800000334060204cadi5204599953038405802KH5914PHAL BRILLIANT6010Phnom Penh9917001317144495763586304B4A7",
    
};
static char* name[] = {
    "Rithy SOURN", "SOURN Rithy", "Brilliant Phal",
};
static char* priceriel = "0 riel";

static char* pricedollar[] = {
    "$ 10", "$ 10.00", "$0"
}; 
static bool s_currency = 0;
static bool s_swipe_r = 0; 
static int8_t s_swipe_bank = 0;
static int8_t s_menu = 0 ; 
 static lv_point_t line_points[5];
///
static bool s_create_b = 0 ;
static bool s_rect_b = 0; 

/**
 * @brief i2c master initialization
 */

static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}
// extern lv_obj_t *scr;
void example_touchpad_read( lv_indev_drv_t * drv, lv_indev_data_t * data )
{
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;

    /* Read touch controller data */
    esp_lcd_touch_read_data(drv->user_data);

    /* Get coordinates */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(drv->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PR;
        ESP_LOGI(TAG_LCD, "X=%u Y=%u", data->point.x, data->point.y);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG_LCD, "Clicked");
    }
    else if(code == LV_EVENT_VALUE_CHANGED) {
        ESP_LOGI(TAG_LCD,"Toggled");
    }
}
static void bank_popup_disp(uint8_t state_bank, char* name, bool currency);
static void bank_disp(uint8_t state_bank, char* name, bool currency,int16_t posx, int16_t posy);
static void bank_hide_disp(uint8_t state_bank, char* name, bool currency,int16_t posx, int16_t posy);
static void delete_object_timer(lv_timer_t *timer){
    lv_obj_del(rect_home_visible);
}

static void cb_time_disp_img(lv_timer_t *timer){
   
    
    // bank_img_hide = lv_img_create(lv_scr_act());
        // ... (configure and display the new image as before) ...
    // lv_img_set_src(bank_img_hide, &aba_pay);
    // lv_obj_del(rect_home_visible);
    int len_bank = sizeof(list_bank)/ sizeof(list_bank[0]);
     ESP_LOGI(TAG_LCD, "SIZEQR: %d",len_bank);
    if (s_swipe_bank > len_bank-1){
        s_swipe_bank = 0 ;
    }
    else if (s_swipe_bank < 0) {
        s_swipe_bank = len_bank -1 ;
    }
    bank_popup_disp(s_swipe_bank,name[s_swipe_bank],s_currency);
   

    // lv_obj_align(bank_img_hide, LV_ALIGN_CENTER, 0, -310);
    // lv_qrcode_update(qr, qrString[1], strlen(qrString[1]));
}


static void cb_time_disp_qr(lv_timer_t *timer){
   
    
    // lv_obj_set_height(qr,0);
    // lv_obj_set_height(currency_img,0);
}
static void set_width(void * var, int32_t v)
{
    lv_obj_set_width((lv_obj_t *)var, v);
}

static void set_height(void * var, int32_t v)
{
    lv_obj_set_height((lv_obj_t *)var, v);


}
static void drag_event_handler(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);

    lv_indev_t * indev = lv_indev_get_act();
    if(indev == NULL)  return;
    int16_t x_pressing= lv_obj_get_x(obj);
    // int16_t y_pressing = lv_obj_get_height(qr);
    // int16_t c_h = lv_obj_get_height(currency_img);
    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);
    lv_coord_t x ;
    lv_coord_t y;
   
    switch(lv_event_get_code(e)) {
        case LV_EVENT_PRESSING:

            x = lv_obj_get_x(obj)+vect.x;
            y = lv_obj_get_y(obj)+abs(vect.y);    
            ESP_LOGI(TAG_LCD, "LINE X=%d",   vect.x );
            
            ESP_LOGI(TAG_LCD, "move x: %d", x_pressing - x );
            ESP_LOGI(TAG_LCD,"y pressing: %d",y);
           
            
            // if(y > 50){
            //     lv_obj_set_height(qr,lv_obj_get_height(qr)-y*1);
            //     lv_obj_set_height(currency_img,lv_obj_get_height(currency_img)-y*1);
            //     ESP_LOGI(TAG_LCD,"y: %d", y);
                
            //      if (y > 60){
            //         if (s_swipe_r)break;
            //         s_swipe_r = 1;
                    
            //         s_currency = !s_currency;
                
            //     if(!s_currency) {
            //         lv_obj_set_height(qr,0);
            //         lv_obj_set_height(currency_img,0);
                    
            //          lv_timer_t * timerqr = lv_timer_create(cb_time_disp_qr, 300, NULL);
            //         ESP_LOGI(TAG_LCD, "currency: riel");
                
            //     lv_qrcode_update(qr, rielqr[s_swipe_bank], strlen(rielqr[s_swipe_bank]));
            //     lv_img_set_src(currency_img, &riel);
            //     lv_label_set_text(t_price, priceriel);
                
                
            //     }
            //     else{
            //         lv_obj_set_height(qr,0);
            //         lv_obj_set_height(currency_img,0);
                    
            //          lv_timer_t * timerqr = lv_timer_create(cb_time_disp_qr, 300, NULL);
            //          ESP_LOGI(TAG_LCD, "currency: dollar");
            //     lv_qrcode_update(qr, dollarqr[s_swipe_bank], strlen(dollarqr[s_swipe_bank]));
            //     lv_img_set_src(currency_img, &dollar);
            //     lv_label_set_text(t_price, pricedollar[s_swipe_bank]);
              
            //     }
            //   } 
            // }
            
             if(x<(-10)){
                lv_obj_set_pos(obj, x, -24);
                if (s_rect_b == 0){
                    if( state_create == 0){
                        bank_disp(0, "Hello", 0, 500,-24);
                        state_create =1;
                        }
                    
                        lv_obj_set_pos(rect_home_visible, 500+x,-24);
                }
                else { 
                    if( state_create == 0){
                        bank_hide_disp(0, "Hello", 0, 500,-24);
                        state_create =1;
                        }
                    
                        lv_obj_set_pos(rect_home_visible, 500+x,-24);
                }
             }
                
              
            
            if((x_pressing - x > 70 || x < (-300))){
                lv_anim_t a;
                lv_anim_init(&a);

                lv_anim_set_var(&a, obj);
                lv_anim_set_values(&a,lv_obj_get_x(obj), -480); 
                lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);  // Move horizontally
                
                lv_anim_set_ready_cb(&a, lv_obj_del_anim_ready_cb); // Delete after animation finishes
                lv_anim_set_time(&a, 3000);  
                lv_anim_start(&a); // panic handler when obj not delete
                lv_obj_del(obj);
                lv_timer_t * timerd = lv_timer_create(delete_object_timer, 300, NULL);
                lv_obj_set_pos(rect_home_hide, abs(x)+500, 0);
                s_swipe_bank++;
                ESP_LOGI(TAG_LCD, "swipe left: %d", s_swipe_bank);
                
                // lv_timer_t * timer = lv_timer_create(cb_time_disp_img, 100, NULL);
                
                // bank_disp(0,"Hello","$ 10",1);
                // vTaskDelay(pdMS_TO_TICKS(310));
               
                // lv_timer_set_repeat_count(timerd, 1);
                // lv_timer_set_repeat_count(timer, 1); // Run only once
            }
            }
            if(x_pressing - x < -50){ //swipe right
                lv_anim_t a;
                lv_anim_init(&a);

                  // lv_anim_set_var(&a, bank_img_disp);
                lv_anim_set_var(&a, obj);
                lv_anim_set_values(&a,lv_obj_get_x(obj), 480); 
                lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);  // Move horizontally
                
                lv_anim_set_ready_cb(&a, lv_obj_del_anim_ready_cb); // Delete after animation finishes
                lv_anim_set_time(&a, 1000);  
                lv_anim_start(&a); // panic handler when obj not delete
                lv_obj_del(obj);
                // lv_timer_t * timerd = lv_timer_create(delete_object_timer, 300, NULL);
                s_swipe_bank = s_swipe_bank -1 ;
                ESP_LOGI(TAG_LCD, "swipe right: %d", s_swipe_bank);
               
            }
            break;
         
        case LV_EVENT_RELEASED:
            // Animate back to the original position
           lv_anim_t b ;
            lv_anim_init(&b);
            lv_anim_set_var(&b, obj);
            lv_anim_set_values(&b, lv_obj_get_x(obj), 0);
            lv_anim_set_exec_cb(&b, (lv_anim_exec_xcb_t)lv_obj_set_x); // Animate x-coordinate
            lv_anim_set_time(&b, 300);  // Animation duration in ms (adjust as needed)
            
            // lv_anim_t qra ;
            // lv_anim_init(&qra);
            // lv_anim_set_var(&qra, qr);
            // lv_anim_set_values(&qra, lv_obj_get_height(qr), 305);
            // lv_anim_set_exec_cb(&qra, (lv_anim_exec_xcb_t)set_height);
            // lv_anim_set_time(&qra, 300);
            // lv_anim_start(&qra);
            // lv_anim_t ca ;
            // lv_anim_init(&ca);
            // lv_anim_set_var(&ca, currency_img);
            // lv_anim_set_values(&ca, lv_obj_get_height(currency_img), 60);
            // lv_anim_set_exec_cb(&ca, (lv_anim_exec_xcb_t)set_height);
            // lv_anim_set_time(&ca, 300);
            lv_anim_start(&b);
            // lv_anim_start(&qra);
            // lv_anim_start(&ca);
            s_swipe_r = 0;
            break;
       
        default:
            break;
            
    }

}
static void drag_release_handler(lv_event_t *e)
{
    lv_obj_t * obj = lv_event_get_target(e);
}


static void bank_hide_disp(uint8_t state_bank, char* name, bool currency, int16_t posx, int16_t posy){
     /* State hide disp  = 1, state display = 0*/
     lv_obj_t * qr_hide;
    rect_home_hide = lv_obj_create(lv_scr_act());
    //   lv_style_set_bg_color(&blue, );
    
    
    // lv_style_set_bg_opa(&rect_home_visible, LV_OPA_COVER); // Set background opacity to fully cover
    
     lv_obj_align(rect_home_hide, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(rect_home_hide,EXAMPLE_LCD_H_RES,750);
    // lv_obj_set_pos(rect_home_hide,500 ,100);
    khqr_bg_hide = lv_img_create(rect_home_hide);
    lv_img_set_src(khqr_bg_hide, &khqr);
    lv_obj_align(khqr_bg_hide, LV_ALIGN_CENTER, 0, 30);
    
  
    t_name_hide = lv_label_create(rect_home_hide);
    lv_obj_add_style(t_name_hide,&style_t_name,0);
    lv_obj_set_style_text_align(t_name_hide,LV_TEXT_ALIGN_LEFT,0);
    
    lv_label_set_text(t_name_hide, name);
// Apply the style to the label
    
   
    t_price_hide = lv_label_create(rect_home_hide);
     lv_obj_add_style(t_price_hide, &style_t_price, 0);
    lv_obj_set_style_text_align(t_price_hide,LV_TEXT_ALIGN_LEFT,0);
    lv_obj_align(t_name_hide, LV_ALIGN_LEFT_MID, 60, -135);
    
    lv_obj_align(t_price_hide, LV_ALIGN_LEFT_MID, 60, -90);
    // // Currency 
    qr_hide = lv_qrcode_create(rect_home_hide, 260,  lv_color_white(), lv_color_black());
    currency_img_hide = lv_img_create(qr_hide);

    lv_obj_align(currency_img_hide, LV_ALIGN_CENTER, 0, 0);
    bank_img_hide = lv_img_create(rect_home_hide);
    lv_obj_align(bank_img_hide, LV_ALIGN_CENTER, 0, -300);
    //  // lv_obj_set_style_transform_angle(qr,2700, 0);

    
    lv_obj_set_style_border_color(qr_hide, lv_color_white(), 0);
    lv_obj_set_style_border_width(qr_hide, 5, 0);
    lv_obj_align(qr_hide, LV_ALIGN_CENTER, 0, 120);
     lv_obj_add_event_cb(rect_home_hide, drag_event_handler, LV_EVENT_ALL, NULL);
     lv_obj_set_pos(rect_home_hide, 0, 22);
    if(!currency) {
        
        lv_img_set_src(currency_img_hide, &riel);
       
        
            
        lv_img_set_src(bank_img_hide,list_bank[state_bank]);
        lv_qrcode_update(qr_hide, rielqr[state_bank], strlen(rielqr[state_bank]));
        lv_label_set_text(t_price_hide, priceriel);

       
    
    }
    else{
        lv_img_set_src(currency_img_hide, &dollar);
     
        lv_img_set_src(bank_img_hide,list_bank[state_bank]);
        lv_qrcode_update(qr_hide, dollarqr[state_bank], strlen(dollarqr[state_bank]));
        lv_label_set_text(t_price_hide, pricedollar[state_bank]);

    }
    //   lv_obj_set_pos(rect_home_hide,200 ,0);
    lv_obj_set_pos(rect_home_hide, 0, -24);
}

static void bank_disp(uint8_t state_bank, char* name, bool currency,int16_t posx, int16_t posy){
     // Currency using boolean when 0 = riel , 1 = dollar ;
   

    rect_home_visible = lv_obj_create(lv_scr_act());
    lv_obj_align(rect_home_visible, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(rect_home_visible,EXAMPLE_LCD_H_RES,750);

    khqr_bg = lv_img_create(rect_home_visible);
    lv_img_set_src(khqr_bg, &khqr);
    lv_obj_align(khqr_bg, LV_ALIGN_CENTER, 0, 30);
 
    t_name = lv_label_create(rect_home_visible);
    lv_obj_add_style(t_name,&style_t_name,0);
    lv_obj_set_style_text_align(t_name,LV_TEXT_ALIGN_LEFT,0);
    
    lv_label_set_text(t_name, name);
  
// Apply the style to the label
    
    t_price = lv_label_create(rect_home_visible);
    lv_obj_add_style(t_price, &style_t_price, 0);
    lv_obj_set_style_text_align(t_price,LV_TEXT_ALIGN_LEFT,0);
    lv_obj_align(t_name, LV_ALIGN_LEFT_MID, 60, -135);
    lv_obj_align(t_price, LV_ALIGN_LEFT_MID, 60, -90);

    // Currency 
    qr = lv_qrcode_create(rect_home_visible, 260,  lv_color_white(), lv_color_black());
   
    currency_img = lv_img_create(qr);

    lv_obj_align(currency_img, LV_ALIGN_CENTER, 0, 0);
    bank_img_disp = lv_img_create(rect_home_visible);
    lv_obj_align(bank_img_disp, LV_ALIGN_CENTER, 0, -300);
   

    lv_obj_set_style_border_color(qr, lv_color_white(), 0);
    lv_obj_set_style_border_width(qr, 5, 0);
    lv_obj_align(qr, LV_ALIGN_CENTER, 0, 120);
     lv_obj_add_event_cb(rect_home_visible, drag_event_handler, LV_EVENT_ALL, NULL);
     lv_obj_set_pos(rect_home_visible, 0, 22);
    if(!currency) {
        
        lv_img_set_src(currency_img, &riel);
        lv_img_set_src(bank_img_disp,list_bank[state_bank]);
        lv_qrcode_update(qr, rielqr[state_bank], strlen(rielqr[state_bank]));
        lv_label_set_text(t_price, priceriel);

       
    
    }
    else{
        lv_img_set_src(currency_img, &dollar);
        lv_img_set_src(bank_img_disp,list_bank[state_bank]);
        lv_qrcode_update(qr, dollarqr[state_bank], strlen(dollarqr[state_bank]));
        lv_label_set_text(t_price, pricedollar[state_bank]);

    }
   lv_obj_set_pos(rect_home_visible, posx, posy);
}
static void bank_popup_disp(uint8_t state_bank, char* name, bool currency){
     // Currency using boolean when 0 = riel , 1 = dollar ;
   
    lv_obj_t * qr;

    rect_home_visible = lv_obj_create(lv_scr_act());
    //   lv_style_set_bg_color(&blue, );
    
    
    // lv_style_set_bg_opa(&rect_home_visible, LV_OPA_COVER); // Set background opacity to fully cover
    
     lv_obj_align(rect_home_visible, LV_ALIGN_CENTER, 0, 0);

    khqr_bg = lv_img_create(rect_home_visible);
    lv_img_set_src(khqr_bg, &khqr);
    lv_obj_align(khqr_bg, LV_ALIGN_CENTER, 0, 30);
    
    
    t_name = lv_label_create(rect_home_visible);
    lv_obj_add_style(t_name,&style_t_name,0);
    lv_obj_set_style_text_align(t_name,LV_TEXT_ALIGN_LEFT,0);
    
    lv_label_set_text(t_name, name);
  
    
    // lv_style_set_text_font(&style_t_name, &lv_font_montserrat_40); 

// Apply the style to the label
    
   
    t_price = lv_label_create(rect_home_visible);
     lv_obj_add_style(t_price, &style_t_price, 0);
    lv_obj_set_style_text_align(t_price,LV_TEXT_ALIGN_LEFT,0);
    lv_obj_align(t_name, LV_ALIGN_LEFT_MID, 60, -135);
    
    lv_obj_align(t_price, LV_ALIGN_LEFT_MID, 60, -90);
    // // Currency 
    qr = lv_qrcode_create(rect_home_visible, 300,  lv_color_white(), lv_color_black());
    currency_img = lv_img_create(qr);

    lv_obj_align(currency_img, LV_ALIGN_CENTER, 0, 0);
    bank_img_disp = lv_img_create(rect_home_visible);
    lv_obj_align(bank_img_disp, LV_ALIGN_CENTER, 0, -300);
    //  // lv_obj_set_style_transform_angle(qr,2700, 0);

    
    lv_obj_set_style_border_color(qr, lv_color_white(), 0);
    lv_obj_set_style_border_width(qr, 5, 0);
    lv_obj_align(qr, LV_ALIGN_CENTER, 0, 120);
     lv_obj_add_event_cb(rect_home_visible, drag_event_handler, LV_EVENT_ALL, NULL);
     lv_obj_set_pos(rect_home_visible, 0, 22);
    if(!currency) {
        
        lv_img_set_src(currency_img, &riel);
       
        
            
        lv_img_set_src(bank_img_disp,list_bank[state_bank]);
        lv_qrcode_update(qr, rielqr[state_bank], strlen(rielqr[state_bank]));
        lv_label_set_text(t_price, priceriel);

       
    
    }
    else{
        lv_img_set_src(currency_img, &dollar);
     
        lv_img_set_src(bank_img_disp,list_bank[state_bank]);
        lv_qrcode_update(qr, dollarqr[state_bank], strlen(dollarqr[state_bank]));
        lv_label_set_text(t_price, pricedollar[state_bank]);

    }
    lv_anim_t a1;
    lv_anim_init(&a1);
    lv_anim_set_var(&a1, rect_home_visible);
    lv_anim_set_values(&a1, 0, 480);
    lv_anim_set_early_apply(&a1, false);
    lv_anim_set_exec_cb(&a1, (lv_anim_exec_xcb_t)set_width);
    lv_anim_set_path_cb(&a1, lv_anim_path_overshoot);
    lv_anim_set_time(&a1, 50);
    lv_anim_t a2;
    lv_anim_init(&a2);
    lv_anim_set_var(&a2, rect_home_visible);
    lv_anim_set_values(&a2,0, 760);
    lv_anim_set_early_apply(&a2, false);
    lv_anim_set_exec_cb(&a2, (lv_anim_exec_xcb_t)set_height);
    lv_anim_set_path_cb(&a2, lv_anim_path_ease_out);
    lv_anim_set_time(&a2, 25);
   
lv_anim_start(&a1);
lv_anim_start(&a2);
    lv_obj_set_pos(rect_home_visible, 0, -24);
//    lv_obj_set_size(rect_home_visible,480,760);
}
static void clear_bank_disp();






////////////////////////////////////////////////////////////////////////////////

static void example_lvgl_touch_cb(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;
   

    /* Read touch controller data */
    esp_lcd_touch_read_data(drv->user_data);

    /* Get coordinates */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(drv->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PR;
        ESP_LOGI(TAG_LCD, "X=%u Y=%u", data->point.x, data->point.y);
        lv_obj_t* scr = lv_line_create(lv_scr_act());
        line_points[0].x =data->point.x;
        line_points[0].y =data->point.y;
        line_points[1].x = data->point.x+10;
        line_points[1].y = data->point.y;
        line_points[2].x = data->point.x+10;
        line_points[2].y = data->point.y+10;
        line_points[3].x = data->point.x;
        line_points[3].y = data->point.y+10;
        line_points[4].x = data->point.x;
        line_points[4].y = data->point.y;
        lv_line_set_points(scr,line_points,5);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

static bool example_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    if (xSemaphoreTakeFromISR(sem_gui_ready, &high_task_awoken) == pdTRUE) {
        xSemaphoreGiveFromISR(sem_vsync_end, &high_task_awoken);
    }
#endif
    return high_task_awoken == pdTRUE;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    xSemaphoreGive(sem_gui_ready);
    xSemaphoreTake(sem_vsync_end, portMAX_DELAY);
#endif
    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

// user funtions 
static void menu_bar()
{
    /* Menu Display */
   

    lv_style_init(&style_label_normal);
    lv_style_init(&style_label_checked);
    lv_style_set_text_font(&style_label_normal, &lv_font_montserrat_24);
    // Set label text colors for better contrast in both states
    lv_style_set_text_color(&style_label_normal, lv_color_hex(0x0));  // Orange-red when unchecked
    lv_style_set_text_color(&style_label_checked, lv_color_hex(0xFFFFFF)); 
    // Button Here
    // Button Menu Style 
   
    
   

    // Initialize the button styles
    lv_style_init(&style_btn_menu);
    lv_style_init(&style_btn_menu_tgl);

    // --- Button Style (Normal State) ---
    lv_style_set_min_height(&style_btn_menu, 70);  // Set height
    lv_style_set_width(&style_btn_menu, 97);         // Set width
           
    lv_style_set_bg_opa(&style_btn_menu, LV_OPA_TRANSP); // No background initially
    lv_style_set_border_width(&style_btn_menu, 2);     // Add a border (stroke)
    lv_style_set_border_color(&style_btn_menu, lv_color_hex(0xCBA02D)); // Set border color
    lv_style_set_radius(&style_btn_menu, 0);
    // --- Button Style (Toggled State) ---
    lv_style_set_bg_opa(&style_btn_menu_tgl, LV_OPA_COVER);   // Solid background when toggled
    lv_style_set_bg_color(&style_btn_menu_tgl, lv_color_hex(0xCBA02D));
    lv_style_set_radius(&style_btn_menu_tgl, 0);
    // List Bank Button 
    b_listbank = lv_btn_create(lv_scr_act());
    
    lv_obj_add_event_cb(b_listbank, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_style(b_listbank, &style_btn_menu, 0);          // Apply default style
    lv_obj_add_style(b_listbank, &style_btn_menu_tgl, LV_STATE_CHECKED); 
    lv_obj_align(b_listbank, LV_ALIGN_BOTTOM_LEFT, 0, -2);
    lv_obj_add_flag(b_listbank, LV_OBJ_FLAG_CHECKABLE);
    // Label
    label_lb = lv_label_create(b_listbank);
    lv_obj_center(label_lb);
    // Create styles for the label
           // White when checked

    // Apply the styles to the label
    lv_obj_add_style(label_lb, &style_label_normal, 0);          // Default style
    lv_obj_add_style(label_lb, &style_label_checked, LV_STATE_CHECKED);
    
    lv_label_set_text(label_lb, LV_SYMBOL_LIST);
    lv_obj_add_flag(label_lb, LV_OBJ_FLAG_CHECKABLE);
    // WIFI QR Button 
    b_qrwifi = lv_btn_create(lv_scr_act());
    
    lv_obj_add_event_cb(b_qrwifi, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_style(b_qrwifi, &style_btn_menu, 0);          // Apply default style
    lv_obj_add_style(b_qrwifi, &style_btn_menu_tgl, LV_STATE_CHECKED); 
    lv_obj_align(b_qrwifi, LV_ALIGN_BOTTOM_LEFT, 96, -2);
    lv_obj_add_flag(b_qrwifi, LV_OBJ_FLAG_CHECKABLE);
    // Label Qr wifi
    label_qrwifi = lv_label_create(b_qrwifi);
    lv_obj_center(label_qrwifi);
    // Create styles for the label
           // White when checked

    // Apply the styles to the label
    lv_obj_add_style(label_qrwifi, &style_label_normal, 0);          // Default style
    lv_obj_add_style(label_qrwifi, &style_label_checked, b_qrwifi);
    lv_label_set_text(label_qrwifi, LV_SYMBOL_WIFI);

    // Home Button


    b_home = lv_btn_create(lv_scr_act());
    
    lv_obj_add_event_cb(b_home, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_style(b_home, &style_btn_menu, 0);          // Apply default style
    lv_obj_add_style(b_home, &style_btn_menu_tgl, LV_STATE_CHECKED); 
    lv_obj_align(b_home, LV_ALIGN_BOTTOM_LEFT, 192, -2);
    lv_obj_add_flag(b_home, LV_OBJ_FLAG_CHECKABLE);
    // Label Home
    label_home = lv_label_create(b_home);
    lv_obj_center(label_home);
  

    // Apply the styles to the label
    lv_obj_add_style(label_home, &style_label_normal, 0);          // Default style
    lv_obj_add_style(label_home, &style_label_checked, b_home);
    lv_label_set_text(label_home, LV_SYMBOL_HOME);

    // Foood Menu Button 
    b_foodmenu = lv_btn_create(lv_scr_act());
    
    lv_obj_add_event_cb(b_foodmenu, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_style(b_foodmenu, &style_btn_menu, 0);          // Apply default style
    lv_obj_add_style(b_foodmenu, &style_btn_menu_tgl, LV_STATE_CHECKED); 
    lv_obj_align(b_foodmenu, LV_ALIGN_BOTTOM_LEFT, 288, -2);
    lv_obj_add_flag(b_foodmenu, LV_OBJ_FLAG_CHECKABLE);
    // Label Food Menu
    label_foodmenu = lv_label_create(b_foodmenu);
    lv_obj_center(label_foodmenu);
    // Create styles for the label
           // White when checked

    // Apply the styles to the label
    lv_obj_add_style(label_foodmenu, &style_label_normal, 0);          // Default style
    lv_obj_add_style(label_foodmenu, &style_label_checked, b_foodmenu);
    lv_label_set_text(label_foodmenu, LV_SYMBOL_EDIT);

    // Setting  Button 
    b_setting = lv_btn_create(lv_scr_act());
    
    lv_obj_add_event_cb(b_setting, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_style(b_setting, &style_btn_menu, 0);          // Apply default style
    lv_obj_add_style(b_setting, &style_btn_menu_tgl, LV_STATE_CHECKED); 
    lv_obj_align(b_setting, LV_ALIGN_BOTTOM_LEFT, 383, -2);
    lv_obj_add_flag(b_setting, LV_OBJ_FLAG_CHECKABLE);
    // Label Setting
    label_setting = lv_label_create(b_setting);
    lv_obj_center(label_setting);   
    // Create styles for the label
           // White when checked

    // Apply the styles to the label
    lv_obj_add_style(label_setting, &style_label_normal, 0);          // Default style
    lv_obj_add_style(label_setting, &style_label_checked, b_setting);
    lv_label_set_text(label_setting, LV_SYMBOL_SETTINGS);
}



void app_main(void)
{
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions

#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    ESP_LOGI(TAG, "Create semaphores");
    sem_vsync_end = xSemaphoreCreateBinary();
    assert(sem_vsync_end);
    sem_gui_ready = xSemaphoreCreateBinary();
    assert(sem_gui_ready);
#endif
 // Initialize NVS (if using it for configuration, not shown here)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    
#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG_LCD, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif
    
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG_LCD, "I2C initialized successfully");
    
    esp_lcd_touch_handle_t tp = NULL;
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    ESP_LOGI(TAG_LCD, "Initialize touch IO (I2C)");
    /* Touch IO handle */
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_MASTER_NUM, &tp_io_config, &tp_io_handle));
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_V_RES,
        .y_max = EXAMPLE_LCD_H_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = 4,
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    /* Initialize touch */
    ESP_LOGI(TAG_LCD, "Initialize touch controller GT911");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp));

    // int ret;
    uint8_t write_buf = 0x01;

    ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x24, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    ESP_LOGI(TAG_LCD,"0x48 0x01 ret is %d",ret);

    write_buf = 0x0E;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    ESP_LOGI(TAG_LCD,"0x70 0x00 ret is %d",ret);

    ESP_LOGI(TAG_LCD, "Install RGB LCD panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .psram_trans_align = 64,
        .num_fbs = EXAMPLE_LCD_NUM_FB,
#if CONFIG_EXAMPLE_USE_BOUNCE_BUFFER
        .bounce_buffer_size_px = 10 * EXAMPLE_LCD_H_RES,
#endif
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = EXAMPLE_PIN_NUM_DISP_EN,
        .pclk_gpio_num = EXAMPLE_PIN_NUM_PCLK,
        .vsync_gpio_num = EXAMPLE_PIN_NUM_VSYNC,
        .hsync_gpio_num = EXAMPLE_PIN_NUM_HSYNC,
        .de_gpio_num = EXAMPLE_PIN_NUM_DE,
        .data_gpio_nums = {
            EXAMPLE_PIN_NUM_DATA0,
            EXAMPLE_PIN_NUM_DATA1,
            EXAMPLE_PIN_NUM_DATA2,
            EXAMPLE_PIN_NUM_DATA3,
            EXAMPLE_PIN_NUM_DATA4,
            EXAMPLE_PIN_NUM_DATA5,
            EXAMPLE_PIN_NUM_DATA6,
            EXAMPLE_PIN_NUM_DATA7,
            EXAMPLE_PIN_NUM_DATA8,
            EXAMPLE_PIN_NUM_DATA9,
            EXAMPLE_PIN_NUM_DATA10,
            EXAMPLE_PIN_NUM_DATA11,
            EXAMPLE_PIN_NUM_DATA12,
            EXAMPLE_PIN_NUM_DATA13,
            EXAMPLE_PIN_NUM_DATA14,
            EXAMPLE_PIN_NUM_DATA15,
        },
        .timings = {
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
            .h_res = EXAMPLE_LCD_H_RES,
            .v_res = EXAMPLE_LCD_V_RES,
            // The following parameters should refer to LCD spec
            .hsync_back_porch = 30,
            .hsync_front_porch = 12,
            .hsync_pulse_width = 6,
            .vsync_back_porch = 30,
            .vsync_front_porch = 12,
            .vsync_pulse_width = 1,
            .flags.pclk_active_neg = true,
        },
        .flags.fb_in_psram = true, // allocate frame buffer in PSRAM
    };
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    ESP_LOGI(TAG_LCD, "Register event callbacks");
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = example_on_vsync_event,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, &disp_drv));

    ESP_LOGI(TAG_LCD, "Initialize RGB LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG_LCD, "Turn on LCD backlight");
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);
#endif

    ESP_LOGI(TAG_LCD, "Initialize LVGL library");
    lv_init();
    void *buf1 = NULL;
    void *buf2 = NULL;
#if CONFIG_EXAMPLE_DOUBLE_FB
    ESP_LOGI(TAG, "Use frame buffers as LVGL draw buffers");
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES);
#else
    ESP_LOGI(TAG_LCD, "Allocate separate LVGL draw buffers from PSRAM");
    buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 160 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    // buf2 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 80 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    // assert(buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * 160 );
#endif // CONFIG_EXAMPLE_DOUBLE_FB

    ESP_LOGI(TAG_LCD, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.full_refresh = true;
    // disp_drv.sw_rotate = 1;
    // disp_drv.rotated = LV_DISP_ROT_90;
	
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    
    // disp_drv.sw_rotate = 0;
   

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
    

    ESP_LOGI(TAG_LCD, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
   

    ESP_LOGI(TAG_LCD,"Register display indev to LVGL");
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init ( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = example_touchpad_read;
    indev_drv.user_data = tp;
    lv_indev_drv_register( &indev_drv );
    

    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));
    
    ESP_LOGI(TAG_LCD, "Display LVGL Scatter Chart");
   
    // default background display 

    lv_style_t style_no_scroll;
     // Text Style 
     lv_style_init(&style_t_name);
     lv_style_init(&style_t_price);

   
    
    lv_style_set_text_font(&style_t_name, &lv_font_montserrat_22);
    lv_style_set_text_align(&style_t_name,LV_TEXT_ALIGN_LEFT);

   
    lv_style_set_text_font(&style_t_price, &lv_font_montserrat_40);

    lv_style_init(&style_no_scroll);
    scr = lv_disp_get_scr_act(disp);
    // lv_obj_set_size(scr, 500*3, (EXAMPLE_LCD_H_RES+20)*3);
    lv_obj_set_style_bg_color(scr, lv_color_white() , 0);
    lv_style_set_pad_all(&style_no_scroll, 0);          // Remove padding to prevent hidden scrollbars
    lv_obj_set_scroll_snap_x(scr, LV_SCROLL_SNAP_NONE); // Disable horizontal scroll snap
    lv_obj_set_scroll_snap_y(scr, LV_SCROLL_SNAP_NONE);
    lv_obj_add_style(scr, &style_no_scroll, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);   // Disable scrollability
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    menu_bar();
    //  bank_popup_disp(s_swipe_bank,name[s_swipe_bank],s_currency);
    
    
    bank_hide_disp(s_swipe_bank,name[s_swipe_bank],s_currency,0,-24);
    /*Set data*/

    // lv_obj_add_event_cb(lv_scr_act(),swipeable_obj_event_cb,LV_EVENT_ALL,NULL);
    
    while (1) {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }
}
