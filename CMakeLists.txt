cmake_minimum_required(VERSION 3.16)

project(MyProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------- SOURCES ----------------
set(SOURCES
    src/main.cpp
    src/Agent.cpp
    src/LinkFetcher
)

add_executable(MyProject ${SOURCES})

# ---------------- LLAMA ----------------
set(LLAMA_DIR /home/ronen/llama.cpp)

add_subdirectory(${LLAMA_DIR} llama_build)

# ---------------- INCLUDE PATHS ----------------
target_include_directories(MyProject PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${LLAMA_DIR}
    ${LLAMA_DIR}/include
    ${LLAMA_DIR}/ggml/include
)

# ---------------- CURL ----------------
find_package(CURL REQUIRED)

target_link_libraries(MyProject PRIVATE CURL::libcurl)

# ---------------- NLOHMANN JSON ----------------
find_package(nlohmann_json REQUIRED)

target_link_libraries(MyProject PRIVATE nlohmann_json::nlohmann_json)

# ---------------- LLAMA LIB ----------------
target_link_libraries(MyProject PRIVATE llama)