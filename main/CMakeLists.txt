
file(GLOB_RECURSE SOURCES "../components/lvgl__lvgl/examples/libs/qrcode/lv_example_qrcode_1.c")
idf_component_register(SRCS "main.c" "lvgl_demo_ui.c" "./components/duck.c" "./components/khqr.c" "./components/dollar.c" "./components/aba_pay.c" "./components/acleda_white.c" "./components/canadia.c" "./components/riel.c" "./components/dollar_red.c" "./components/riel_red.c" ${SOURCES}
                       INCLUDE_DIRS "." "../components/espressif__esp_lcd_touch_gt911/include" "./components")
