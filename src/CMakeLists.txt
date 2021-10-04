if(NOT EXISTS "${CMAKE_BINARY_DIR}/src/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                "${CMAKE_BINARY_DIR}/src/conan.cmake"
                EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
                TLS_VERIFY ON)
endif()

include(${CMAKE_BINARY_DIR}/src/conan.cmake)
include(${CMAKE_BINARY_DIR}/src/conanbuildinfo.cmake)

CONAN_BASIC_SETUP(TARGETS)

include_directories(${CONAN_INCLUDE_DIRS_BOOST})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(Common)
add_subdirectory(CParserFrontend)
add_subdirectory(Generator)
add_subdirectory(Parser)