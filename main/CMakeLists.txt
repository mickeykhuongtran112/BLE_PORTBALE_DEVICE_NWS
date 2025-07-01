idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "" "${CMAKE_SOURCE_DIR}/components" 
                    REQUIRES bt
                    rfid_module         
                    esp_wifi           
                    nvs_flash        
                    mqtt                
                    esp_event     
                    main     
                    driver
                    flash
                    ble
                    button
                    )
# idf_component_register(SRCS "main.c"
