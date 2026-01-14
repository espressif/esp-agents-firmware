idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_wifi esp_driver_touch_sens esp_lcd console
    EMBED_FILES "${CMAKE_CURRENT_LIST_DIR}/../assets/audio/wakeup.mp3"
    EMBED_FILES "${CMAKE_CURRENT_LIST_DIR}/../assets/audio/finish_reminder.mp3"
)
