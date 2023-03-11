if(NOT EXISTS ${CMAKE_BINARY_DIR}/src/conanbuildinfo.cmake)
	execute_process(COMMAND conan install ${CMAKE_SOURCE_DIR}/src/conanfile.txt -if ${CMAKE_BINARY_DIR}/src --build=missing -s build_type=${CMAKE_BUILD_TYPE})
endif()

include(${CMAKE_BINARY_DIR}/src/conanbuildinfo.cmake)

CONAN_BASIC_SETUP(TARGETS)

include_directories(${CONAN_INCLUDE_DIRS_BOOST})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR})

# 用户规约函数的构建脚本
include(Config/UserLibrariesConstructConfig/CMakeLists.txt)

add_subdirectory(Common)
add_subdirectory(Config)
add_subdirectory(Generator)
add_subdirectory(Parser)