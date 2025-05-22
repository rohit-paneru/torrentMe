cmake_minimum_required(VERSION 3.10)
project(MiniTorrent)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add common source files
set(COMMON_SOURCES
    common.cpp
    network.cpp
    fileutils.cpp
)

# Windows-specific libraries
if(WIN32)
    set(PLATFORM_LIBS ws2_32)
endif()

# Tracker executable
add_executable(tracker tracker.cpp ${COMMON_SOURCES})
target_link_libraries(tracker ${PLATFORM_LIBS})

# Peer executable
add_executable(peer peer.cpp ${COMMON_SOURCES})
target_link_libraries(peer ${PLATFORM_LIBS})

# Add threading support
find_package(Threads REQUIRED)
target_link_libraries(tracker Threads::Threads)
target_link_libraries(peer Threads::Threads)