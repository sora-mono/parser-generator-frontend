include_directories(${CMAKE_SOURCE_DIR}/src)

aux_source_directory(. CPARSERFRONTEND_SRC)

add_library(c_parser_frontend ${CPARSERFRONTEND_SRC})
target_compile_options(c_parser_frontend PRIVATE /std:c++latest)
target_link_libraries(c_parser_frontend line_and_column)