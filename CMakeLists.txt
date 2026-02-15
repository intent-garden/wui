cmake_minimum_required (VERSION 3.5)

project (wui CXX C)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -static-libstdc++ -static-libgcc")

set(CMAKE_MAKE_PROGRAM "make -j 3")

#set(CMAKE_CXX_FLAGS:STRING= -fsanitize=address  -fsanitize=leak -g)
#set(CMAKE_C_FLAGS:STRING=-fsanitize=address  -fsanitize=leak -g)
#set(CMAKE_EXE_LINKER_FLAGS:STRING=-fsanitize=address  -fsanitize=leak)
#set(CMAKE_MODULE_LINKER_FLAGS:STRING=-fsanitize=address  -fsanitize=leak)

include_directories(
	include)

link_directories(
	src)

add_subdirectory (src)
add_subdirectory (examples/hello_world)
add_subdirectory (examples/simple)
add_subdirectory (examples/demo)
