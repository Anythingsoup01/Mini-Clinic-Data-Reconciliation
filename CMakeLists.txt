cmake_minimum_required(VERSION 4.2.3)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(mini-clinic-wks LANGUAGES CXX)
set(SRCS
    "src/llm_api.cpp"
    "src/main.cpp"
    "src/webserver.cpp"
)
add_executable(mini-clinic ${SRCS})
set_property(TARGET mini-clinic PROPERTY CXX_STANDARD 20)

target_include_directories(mini-clinic PRIVATE
    "include"
    "vendor/yaml-cpp/include"
)

target_link_libraries(mini-clinic
    curl
    yaml-cpp
)

add_subdirectory(vendor/yaml-cpp)
