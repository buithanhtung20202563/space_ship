cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME space_ship)
project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Audio)
file (GLOB SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "include")

target_link_libraries(${PROJECT_NAME} 
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    pthread
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})