## CMake (Windows)

Install CMake: <http://www.cmake.org>

MSVC Win32 Debug

     $ md build && cd build
     $ cmake -DCMAKE_INSTALL_PREFIX=./install -DCMAKE_DEBUG_POSTFIX="_d" -G "Visual Studio 12" .. 
     $ msbuild INSTALL.vcxproj /t:Build /p:Configuration=Debug

MSVC Win64 Release 

     $ md build && cd build
     $ cmake -DCMAKE_INSTALL_PREFIX=./install -G "Visual Studio 12 Win64" .. 
     $ msbuild INSTALL.vcxproj /t:Build /p:Configuration=Release

## CMake (Unix)

     $ mkdir build && cd build
     $ cmake -DCMAKE_INSTALL_PREFIX=$PWD/install -DCMAKE_BUILD_TYPE=Debug -DCMAKE_DEBUG_POSTFIX="_d" ..     # Default to Unix Makefiles.
     $ make
     $ make install

## CMake (Android)

     $ mkdir build && cd build
     $ cmake \ 
            -DCMAKE_INSTALL_PREFIX=$PWD/install \
            -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_DEBUG_POSTFIX="_d" \
            -DCMAKE_TOOLCHAIN_FILE=android.toolchain.cmake \
            -G "Unix Makefiles" ..
     $ make
     $ make install
