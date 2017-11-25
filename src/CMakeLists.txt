cmake_minimum_required(VERSION 2.8)

project(liblog4x)

#set(SOURCES log4x.cpp file.cpp)
aux_source_directory(. SOURCES)

set(HEADERS log4x.h)

if(MSVC)
    add_definitions(-DLOG4X_EXPORTS)
endif()

if(BUILD_SHARED_LIBS)
    add_library(log4x SHARED ${SOURCES} ${HEADERS})
    install(TARGETS log4x DESTINATION lib)
endif()

if(BUILD_STATIC_LIBS)
    add_library(log4x_static STATIC ${SOURCES} ${HEADERS})
    set_target_properties(log4x_static PROPERTIES OUTPUT_NAME "log4x")
    install(TARGETS log4x_static DESTINATION lib)
endif()

install(FILES ${HEADERS} DESTINATION include)
