include(${CMAKE_BINARY_DIR}/src/conanbuildinfo.cmake)

CONAN_BASIC_SETUP(TARGETS)

include_directories(${CONAN_INCLUDE_DIRS_BOOST})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(Common)
add_subdirectory(CParserFrontend)
add_subdirectory(Generator)
add_subdirectory(Parser)