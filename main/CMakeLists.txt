file(GLOB srcs_WIFI "${CMAKE_CURRENT_LIST_DIR}/WIFI_Smart/*.c")
file(GLOB srcs_OTA "${CMAKE_CURRENT_LIST_DIR}/HTTPS_OTA/*.c")

idf_component_register(SRCS "${srcs_WIFI}" "${srcs_OTA}" "main.c"
                    INCLUDE_DIRS "." "WIFI_Smart" "HTTPS_OTA"
                    EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)