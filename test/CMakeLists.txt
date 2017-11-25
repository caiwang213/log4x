cmake_minimum_required(VERSION 2.8)

include_directories(${LOG4X_SRC_DIR})
link_directories(${LOG4X_SRC_DIR})

set(CONFIGS log.ini)

if(BUILD_SHARED_LIBS)
    add_executable(log4x_test main.cpp)
    target_link_libraries(log4x_test log4x)
    install(TARGETS log4x_test DESTINATION bin)
endif()

if(BUILD_STATIC_LIBS)
    add_executable(log4x_test_static main.cpp)
    add_definitions(-DLOG4X_STATICLIB)
    target_link_libraries(log4x_test_static log4x_static)
    #set_target_properties(log4x_test_static PROPERTIES OUTPUT_NAME "log4x_test_static")
    install(TARGETS log4x_test_static DESTINATION bin)
endif()

install(FILES ${CONFIGS} DESTINATION bin)
