#usage http://stackoverflow.com/questions/5098360/cmake-specifying-build-toolchain
#cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake
INCLUDE(CMakeForceCompiler)

set (CMAKE_SYSTEM_NAME Generic)
set (CMAKE_C_COMPILER arm-elf-eabi-gcc)

CMAKE_FORCE_C_COMPILER(arm-elf-eabi-gcc GNU)
#CMAKE_FORCE_CXX_COMPILER(arm-elf-eabi-g++ GNU)

#for linker script see
#http://stackoverflow.com/questions/32864689/cmake-how-to-add-dependency-on-linker-script-for-executable
#set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})
