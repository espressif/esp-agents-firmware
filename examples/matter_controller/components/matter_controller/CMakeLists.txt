set(ldfragments linker.lf)
set(src_dirs)
set(include_dirs)
if (CONFIG_ESP_CLIENT_ONLY_MATTER_CONTROLLER)
    list(APPEND src_dirs "rmaker_controller_service" "op_creds_issuer")
    list(APPEND include_dirs "rmaker_controller_service" "op_creds_issuer")
endif()
idf_component_register(SRC_DIRS ${src_dirs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES esp_matter esp_rainmaker controller_rest_apis esp_rcp_update
                       LDFRAGMENTS "${ldfragments}")
