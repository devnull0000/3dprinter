cmake_minimum_required (VERSION 3.7.0)
project (printctl)

#add_definitions(-DSTM32F765xx -DUSE_USB_FS)

include_directories (
src/
../
)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

#--specs=nosys.specs to compiler
set (SRC_FILES 
src/main.cpp
)

add_executable (${PROJECT_NAME}.elf ${SRC_FILES}) 
