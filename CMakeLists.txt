cmake_minimum_required(VERSION 3.10)

project(searxng_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- CURL ----
find_package(CURL REQUIRED)

# ---- Gumbo (manual linking, no pkg-config) ----
find_library(GUMBO_LIB gumbo REQUIRED)

# ---- PostgreSQL (libpqxx) ----
find_library(PQXX_LIB pqxx REQUIRED)
find_library(PQ_LIB pq REQUIRED)

# ---- Executable ----
add_executable(searx_scraper
    src/main.cpp
    src/searxng_client.cpp
    src/scraper.cpp
)

# ---- Include directories ----
target_include_directories(searx_scraper PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/bloom   # 👈 ADD THIS (where bloom_filter.hpp is)
)

# ---- Link libraries ----
target_link_libraries(searx_scraper PRIVATE
    CURL::libcurl
    ${GUMBO_LIB}
    ${PQXX_LIB}
    ${PQ_LIB}
)