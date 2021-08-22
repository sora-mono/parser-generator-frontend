include_directories(${CMAKE_SOURCE_DIR}/src)

aux_source_directory(. CPARSERFRONTEND_SRC)

add_library(c_parser_frontend ${CPARSERFRONTEND_SRC}    "reduct_functions.h" "reduct_functions.cpp")