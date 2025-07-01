set(app_src rfid_module.c ) #wifi_connect.c
set(pri_req)
idf_component_register(SRCS ${app_src}
                    REQUIRES ${pri_req}
                    INCLUDE_DIRS "." "${CMAKE_SOURCE_DIR}/main"
                    REQUIRES main flash ble 
                  )
                  