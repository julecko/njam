cmake_minimum_required(VERSION 3.10)
project(njam C)

set(NJAM_VERSION "1.0.0")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set default to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Enable link-time optimization (interprocedural)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)


# Debug/Release compiler flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_MODE=1)
    add_compile_options(-g -O0)
else()
    add_compile_definitions(NDEBUG)
    add_compile_options(-O3)
endif()

# Executable 1: njam
add_executable(njam
    src/main.c
    src/ip_utils.c
    src/mask_utils.c
    src/network.c
    src/icmp.c
    src/arp.c
    src/process/spoofer.c
    src/process/scanner.c
)

target_include_directories(njam PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(njam PUBLIC VERSION=\"${NJAM_VERSION}\")


# Prepare dpkg packaging
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CPACK_PACKAGE_NAME "njam")
    set(CPACK_PACKAGE_VERSION "${NJAM_VERSION}")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Julecko <julecko@dilino.sk>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Network jammer utilizing arp spoofing")

    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libc6 (>= 2.28)"
    )

    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_SOURCE_DIR}/packaging/postinst"
    )


    include(CPack)

    install(TARGETS njam DESTINATION bin)

endif()