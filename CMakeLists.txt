cmake_minimum_required(VERSION 3.16)

project(MyProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------- SOURCES ----------------
set(SOURCES
    src/main.cpp
    src/Agent.cpp
    src/LinkFetcher.cpp
    src/SiteScraper.cpp
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

# ---------------- JSON ----------------
find_package(nlohmann_json REQUIRED)
target_link_libraries(MyProject PRIVATE nlohmann_json::nlohmann_json)

# ---------------- GUMBO ----------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUMBO REQUIRED gumbo)

target_include_directories(MyProject PRIVATE
    ${GUMBO_INCLUDE_DIRS}
)

target_link_libraries(MyProject PRIVATE
    ${GUMBO_LIBRARIES}
)

# ---------------- LLAMA LIB ----------------
target_link_libraries(MyProject PRIVATE llama)