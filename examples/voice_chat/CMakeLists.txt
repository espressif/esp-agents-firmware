cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(SDKCONFIG_DEFAULTS ${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults)
include(${CMAKE_CURRENT_LIST_DIR}/../common/boards/add_board_sdkconfig_defaults.cmake)

set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../common"
                         "${CMAKE_CURRENT_LIST_DIR}/../../components")

project(voice_chat)
