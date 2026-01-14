# Check if a board is selected. Either using `idf.py select-board` or `idf.py gen_bmgr_config`
# Selected board name will be saved by `idf_ext_boards.py`
set(GEN_BMGR_CODES_PATH "${CMAKE_CURRENT_BINARY_DIR}/../components/gen_bmgr_codes")
set(AGENT_BOARD_NAME_FILE "${GEN_BMGR_CODES_PATH}/agent_board_name.txt")

if(NOT EXISTS ${AGENT_BOARD_NAME_FILE} AND NOT EXISTS ${GEN_BMGR_CODES_PATH})
    message(FATAL_ERROR "Please select a board using `idf.py select-board --board <board_name>` before proceeding further")
endif()

# add board-specific SDKCONFIG_DEFAULTS
if(EXISTS ${AGENT_BOARD_NAME_FILE})
    file(READ ${AGENT_BOARD_NAME_FILE} BOARD_NAME)
    string(STRIP ${BOARD_NAME} BOARD_NAME)
    set(BOARD_PATH "${CMAKE_CURRENT_LIST_DIR}/${BOARD_NAME}")
    if(EXISTS ${BOARD_PATH}/sdkconfig.defaults)
        list(APPEND SDKCONFIG_DEFAULTS ${BOARD_PATH}/sdkconfig.defaults)
        message(STATUS "Added ${BOARD_PATH}/sdkconfig.defaults to SDKCONFIG_DEFAULTS")
    endif()
endif()
