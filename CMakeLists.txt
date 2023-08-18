CMAKE_MINIMUM_REQUIRED(VERSION 3.16) 

set(CMAKE_CXX_STANDARD 14)

project(MyWebServer)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/src/include) 

aux_source_directory(${CMAKE_SOURCE_DIR}/src SOURCES)

add_executable(${PROJECT_NAME} src/main.cpp ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -g)

target_link_libraries(${PROJECT_NAME} PRIVATE pthread mysqlclient)


