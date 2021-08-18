cmake_minimum_required(VERSION 3.8)

set(APP_NAME "ScheduleDemo")
project(${APP_NAME} CXX)

set(PROJ_SOURCE 
    ${CMAKE_CURRENT_LIST_DIR}/source/Main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/source/Tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/source/core/Scheduler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/source/core/Scheduler.h
    ${CMAKE_CURRENT_LIST_DIR}/source/core/System.h
)
set(PROJ_SOURCE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/source    
)
add_executable(${APP_NAME}
    ${PROJ_SOURCE}
)
target_include_directories(${APP_NAME} PUBLIC
    ${PROJ_SOURCE_DIR}
)

