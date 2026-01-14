set(COMPONENT_DIRS "audio_playback" "audio_recorder")

set(INCLUDE_DIRS ${COMPONENT_DIRS})
set(SRC_DIRS ${COMPONENT_DIRS} ".")

idf_component_register(
    SRC_DIRS ${SRC_DIRS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    PRIV_INCLUDE_DIRS priv_include
    REQUIRES esp_psram
)
