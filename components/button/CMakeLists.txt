set(app_src gpio.c)
set(pri_req)
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS "." "${CMAKE_SOURCE_DIR}/main"
                    REQUIRES main driver rfid_module
            )