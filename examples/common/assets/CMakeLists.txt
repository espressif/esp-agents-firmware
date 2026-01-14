idf_component_register()

# Build the partition for animation and icon image files
set(ASSETS_NAME "asset_test")
if(CONFIG_BOARD_ECHOEAR_CORE_BOARD_V1_2)
    set(RESOLUTION "360_360")
elseif(CONFIG_BOARD_M5STACK_CORES3 OR CONFIG_BOARD_ESP_BOX_3)
    set(RESOLUTION "320_240")
else()
    message(STATUS "Unknown board, using default")
    set(RESOLUTION "320_240")
endif()
message(STATUS "Resolution: ${RESOLUTION}")

set(ASSETS_FILE "${CMAKE_BINARY_DIR}/${ASSETS_NAME}.bin")
build_speaker_assets_bin("anim_icon" ${RESOLUTION} ${ASSETS_FILE} ${CONFIG_MMAP_FILE_NAME_LENGTH})
message(STATUS "Generated emote assets: ${ASSETS_FILE} -> anim_icon partition")
esptool_py_flash_to_partition(flash "anim_icon" "${ASSETS_FILE}")
