cmake_minimum_required(VERSION 3.16)
project(netpulse C)

# Use default C standard (compiler default, typically C17 or newer)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# Platform detection
if(APPLE)
    add_definitions(-DPLATFORM_MACOS)
    set(PLATFORM_LIBS "")
elseif(UNIX)
    add_definitions(-DPLATFORM_LINUX)
    set(PLATFORM_LIBS "pthread")
endif()

# Mongoose configuration
add_definitions(-DMG_ENABLE_LINES=1)
add_definitions(-DMG_ENABLE_DIRECTORY_LISTING=0)

# Source files
set(PLATFORM_SOURCES
    src/platform/time.c
    src/platform/fs.c
)

set(CORE_SOURCES
    src/core/ring_buffer.c
    src/core/config.c
    src/core/stats.c
    src/core/event_log.c
    src/core/scheduler.c
)

set(NET_SOURCES
    src/net/dns.c
    src/net/tcp_probe.c
)

set(SERVER_SOURCES
    src/server/server.c
    src/server/http_handlers.c
    src/server/ws_handlers.c
)

set(THIRD_PARTY_SOURCES
    third_party/mongoose/mongoose.c
)

# Main executable
add_executable(netpulsed
    src/main.c
    ${PLATFORM_SOURCES}
    ${CORE_SOURCES}
    ${NET_SOURCES}
    ${SERVER_SOURCES}
    ${THIRD_PARTY_SOURCES}
)

target_include_directories(netpulsed PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/mongoose
)

target_link_libraries(netpulsed ${PLATFORM_LIBS})

# Install target
install(TARGETS netpulsed DESTINATION bin)
