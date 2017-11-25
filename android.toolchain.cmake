# this is required
SET(CMAKE_SYSTEM_NAME Linux)

SET(CMAKE_TOOLCHAIN_PATH   /usr/local/android-ndk-standalone/r10e/arm-linux-androideabi-4.9)
SET(CMAKE_TOOLCHAIN_PREFIX ${CMAKE_TOOLCHAIN_PATH}/bin/arm-linux-androideabi-)

# specify the cross compiler
SET(CMAKE_C_COMPILER   ${CMAKE_TOOLCHAIN_PREFIX}gcc)
SET(CMAKE_CXX_COMPILER ${CMAKE_TOOLCHAIN_PREFIX}g++)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH ${CMAKE_TOOLCHAIN_PATH}/sysroot/)

# search for programs in the build host directories (not necessary)
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
