cmake_minimum_required(VERSION 3.28)
project(chat_server_2.0)
set(CMAKE_CXX_STANDARD 17)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_path(MYSQL_INCLUDE_DIR
          NAMES mysql.h
          PATH_SUFFIXES mysql)

find_library(MYSQL_LIB
             NAMES mysqlclient)

if(NOT MYSQL_INCLUDE_DIR OR NOT MYSQL_LIB)
    message(FATAL_ERROR "找不到 MySQL 开发库！请确认已安装 libmysqlclient-dev")
endif()

aux_source_directory(${SRC_DIR} CPP)
set(EXECUTABLE_OUTPUT_PATH ${BIN_DIR})
include_directories(${INCLUDE_DIR} ${MYSQL_INCLUDE_DIR})

add_executable(chat_server ${CPP})

target_link_libraries(chat_server ${MYSQL_LIB})




