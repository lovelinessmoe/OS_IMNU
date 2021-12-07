cmake_minimum_required(VERSION 3.16)
project(untitled C)

set(CMAKE_C_STANDARD 99)


add_executable(main main.c)
add_executable(pipe pipe.c)
add_executable(pipeTest pipeTest.c)
add_executable(Test Test.c)
add_executable(softInterrupt softInterrupt.c)
add_executable(msg msg.c)
add_executable(Test2 Test2.c)
add_executable(TestNull TestNull.c)
add_executable(shareStorge shareStorge.c)
add_executable(PageReplacement PageReplacement.c)
add_executable(FileSystem FileSystem.c)