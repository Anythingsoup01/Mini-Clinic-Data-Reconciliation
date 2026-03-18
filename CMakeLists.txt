cmake_minimum_required(VERSION 4.3.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(mini-clinic-wks LANGUAGES CXX)
set(SRCS
    "src/webserver.cpp"
    "src/routes/routes.cpp"
    "src/serializer.cpp"
    "src/main.cpp"
    "src/log.cpp"
    "src/llm_api.cpp"
)
add_executable(mini-clinic ${SRCS})
set_property(TARGET mini-clinic PROPERTY CXX_STANDARD 20)

target_include_directories(mini-clinic PRIVATE
    "include"
)

target_link_libraries(mini-clinic
    curl
)

