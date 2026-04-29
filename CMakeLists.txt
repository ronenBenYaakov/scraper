cmake_minimum_required(VERSION 3.10)

project(searxng_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- CURL ----
find_package(CURL REQUIRED)

# ---- Gumbo ----
find_library(GUMBO_LIB gumbo REQUIRED)

# ---- PostgreSQL ----
find_library(PQXX_LIB pqxx REQUIRED)
find_library(PQ_LIB pq REQUIRED)

# ---- RocksDB ----
find_library(ROCKSDB_LIB rocksdb REQUIRED)

# RocksDB dependencies (IMPORTANT)
find_library(SNAPPY_LIB snappy REQUIRED)
find_library(Z_LIB z REQUIRED)
find_library(LZ4_LIB lz4 REQUIRED)
find_library(BZ2_LIB bz2 REQUIRED)

# =========================================================
# INCLUDE PATHS
# =========================================================
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/bloom
)

# =========================================================
# COMMON LINK LIBS (avoid duplication)
# =========================================================
set(COMMON_LIBS
    CURL::libcurl
    ${GUMBO_LIB}
    ${PQXX_LIB}
    ${PQ_LIB}
    ${ROCKSDB_LIB}
    ${SNAPPY_LIB}
    ${Z_LIB}
    ${LZ4_LIB}
    ${BZ2_LIB}
)

# =========================================================
# CORE EXECUTABLE
# =========================================================
add_executable(searx_scraper
    src/main.cpp
    src/searxng_client.cpp
    src/scraper.cpp
)

target_link_libraries(searx_scraper PRIVATE ${COMMON_LIBS})

