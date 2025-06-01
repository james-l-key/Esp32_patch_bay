idf_component_register(SRCS "config_check.c" "main.c" "gui.c" "matrix.c" "buttons.c" "config_check.c"
                      INCLUDE_DIRS "."
                      REQUIRES "lvgl" "esp_lvgl_port" "nvs_flash")