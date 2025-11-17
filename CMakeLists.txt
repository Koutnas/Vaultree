cmake_minimum_required(VERSION 3.10)
project(vaultree LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
# Find SQLite3
find_package(SQLite3 REQUIRED)
# Include headers in src/ or include/
include_directories(src include)

# DBManager library
add_library(dbmanager
    src/db_manager.cpp
)
target_include_directories(dbmanager PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(dbmanager PRIVATE ${SQLite3_LIBRARIES})

# Versioneer depends on DBManager
add_library(versioneer
    src/versioneer.cpp
)
target_link_libraries(versioneer PRIVATE dbmanager)

# Main executable depends on Versioneer
add_executable(vaultree
    src/main.cpp
)
target_link_libraries(vaultree PRIVATE versioneer)
