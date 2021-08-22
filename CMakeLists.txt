# CMakeList.txt: parser-generator-frontend 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project ("parser-generator-frontend")

add_compile_options("/std:c++latest")

# 将源代码添加到此项目的可执行文件。

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

add_subdirectory("src")

# TODO: 如有需要，请添加测试并安装目标。