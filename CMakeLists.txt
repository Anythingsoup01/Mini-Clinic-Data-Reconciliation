cmake_minimum_required(VERSION 4.2.3)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(mini-clinic-wks LANGUAGES CXX)
set(SRCS
    "src/main.cpp"
    "src/webhost.cpp"
)
add_executable(mini-clinic ${SRCS})
set_property(TARGET mini-clinic PROPERTY CXX_STANDARD 20)

target_include_directories(mini-clinic PRIVATE
    "include"
    "vendor/llama/include"
)

target_link_libraries(mini-clinic
    llama
)

add_subdirectory(vendor/llama)
