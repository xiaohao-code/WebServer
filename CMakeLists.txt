CMAKE_MINIMUM_REQUIRED(VERSION 3.16) 

set(CMAKE_CXX_STANDARD 14)

project(WebServer)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

aux_source_directory(${CMAKE_SOURCE_DIR}/webserver SOURCES)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -g)

target_link_libraries(${PROJECT_NAME} PRIVATE pthread mysqlclient)


