idf_component_register(SRCS "wifi.c" "main.c"
                    INCLUDE_DIRS ".")
                    
target_add_binary_data(${COMPONENT_TARGET} "certs/aws-root-ca.pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/https_certificate.pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/certificate.pem.crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/private.pem.key" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "data.json" TEXT)