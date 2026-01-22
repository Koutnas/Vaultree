cmake_minimum_required(VERSION 3.10)
project(vaultree LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON )
# Find SQLite3
find_package(SQLite3 REQUIRED)
# Include headers in src/ or include/

#blake3_sources
add_library(blake3lib
    third_party/blake3/blake3.c
    third_party/blake3/blake3_portable.c
    third_party/blake3/blake3_dispatch.c
)
target_compile_definitions( blake3lib PRIVATE
    BLAKE3_NO_SSE2
    BLAKE3_NO_SSE41
    BLAKE3_NO_AVX2
    BLAKE3_NO_AVX512
)
target_include_directories(blake3lib PUBLIC third_party/blake3)

add_library(hasher
    src/hasher.cpp
)
target_link_libraries(hasher PUBLIC blake3lib)
target_include_directories(hasher PUBLIC ${CMAKE_SOURCE_DIR}/include)

# DBManager library
add_library(dbmanager
    src/db_manager.cpp
)
target_link_libraries(dbmanager PRIVATE SQLite::SQLite3)
target_include_directories(dbmanager PUBLIC ${CMAKE_SOURCE_DIR}/include)

#worker_pool
add_library(workerpool
    src/worker_pool.cpp
)
target_include_directories(workerpool PUBLIC ${CMAKE_SOURCE_DIR}/include)


# file_processor
add_library(fileprocessor
    src/file_processor.cpp
)
target_link_libraries(fileprocessor PRIVATE dbmanager PRIVATE hasher PRIVATE workerpool)
target_include_directories(fileprocessor PUBLIC ${CMAKE_SOURCE_DIR}/include)
# Versioneer depends on DBManager
add_library(versioneer
    src/versioneer.cpp
)
target_link_libraries(versioneer PRIVATE dbmanager PUBLIC hasher PRIVATE fileprocessor)
target_include_directories(versioneer PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Main executable
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(vaultree
    src/main.cpp
)
target_link_libraries(vaultree PRIVATE versioneer)