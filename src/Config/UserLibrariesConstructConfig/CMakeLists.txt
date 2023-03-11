# 这里设置定义规约函数的文件夹
#add_subdirectory(${CMAKE_SOURCE_DIR}/src/CParserFrontend)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/Brainfuck)

# 这里设置规约函数编译出来的库名，可以设置多个库，但是必须设置到名为"UserLibraires"的变量
#set(UserLibraries c_parser_frontend)
set(UserLibraries brainfuck)