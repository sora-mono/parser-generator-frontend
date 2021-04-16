# CMakeList.txt: C-parser-generator-frontend 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project ("C-parser-generator-frontend")

add_compile_options("/std:c++latest")

# 将源代码添加到此项目的可执行文件。

include_directories("E:/VS shared resources/boost_1_75_0")

add_subdirectory("Generator")
add_subdirectory("Parser")
add_subdirectory("Common")

# TODO: 如有需要，请添加测试并安装目标。

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/out")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/out")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/out")

link_libraries("E:/VS shared resources/boost_1_75_0/lib")
