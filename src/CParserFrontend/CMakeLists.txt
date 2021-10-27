include_directories(${CMAKE_SOURCE_DIR}/src)

aux_source_directory(. CPARSERFRONTEND_SRC)

add_library(c_parser_frontend ${CPARSERFRONTEND_SRC})
target_link_libraries(c_parser_frontend CONAN_PKG::boost line_and_column)