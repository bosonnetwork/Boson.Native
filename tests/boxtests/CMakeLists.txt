include(ProjectDefaults)
include(CheckSymbolExists)

check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
if(HAVE_SYS_RESOURCE_H)
    add_definitions(-DHAVE_SYS_RESOURCE_H=1)
endif()

check_include_file(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
    add_definitions(-DHAVE_UNISTD_H=1)
endif()

check_include_file(dirent.h HAVE_DIRENT_H)
if(HAVE_DIRENT_H)
    add_definitions(-DHAVE_DIRENT_H=1)
endif()

check_symbol_exists(SIGKILL signal.h HAVE_SIGKILL)
if(HAVE_SIGKILL)
    add_definitions(-DHAVE_SIGKILL=1)
endif()

check_symbol_exists(SIGHUP signal.h HAVE_SIGHUP)
if(HAVE_SIGHUP)
    add_definitions(-DHAVE_SIGHUP=1)
endif()

include_directories(
    .
    ../../include
    ../../src/core
    ../common
    ${CARRIER_INT_DIST_DIR}/include)

list(APPEND APITESTS_SOURCES
    main.cc
    ../common/utils.cc
    node_tests.cc
)

set(LIBS
    cppunit)

if(WIN32)
    add_definitions(
        -DWIN32_LEAN_AND_MEAN
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_NONSTDC_NO_WARNINGS)

    set(LIBS
        ${LIBS}
        Ws2_32
        crypt32
        iphlpapi
        Shlwapi)
endif()

list(APPEND APITESTS_DEPENDS
    CLI11
    sqlite
    CppUnit
    carrier0
    libsodium)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(SYSTEM_LIBS pthread dl)
endif()

if(ENABLE_STATIC)
    set(LIBS carrier-static ${LIBS})
endif()

add_executable(boxtests ${APITESTS_SOURCES})
target_link_libraries(boxtests ${LIBS} ${SYSTEM_LIBS})
add_dependencies(boxtests ${APITESTS_DEPENDS})

configure_file(boxtests.conf boxtests.conf COPYONLY)

enable_testing()
add_test(TEST boxtests)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    install(TARGETS boxtests
        RUNTIME DESTINATION "bin"
        ARCHIVE DESTINATION "lib"
        LIBRARY DESTINATION "lib")
endif()

install(FILES boxtests.conf DESTINATION "bin")
