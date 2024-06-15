
file(GLOB_RECURSE SOURCES "../components/lvgl__lvgl/examples/libs/qrcode/lv_example_qrcode_1.c")
idf_component_register(SRCS "main.c" "lvgl_demo_ui.c"  ${SOURCES}
                       INCLUDE_DIRS "." "../components/espressif__esp_lcd_touch_gt911/include" )
