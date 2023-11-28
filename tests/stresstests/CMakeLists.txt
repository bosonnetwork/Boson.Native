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

check_include_file(io.h HAVE_IO_H)
if(HAVE_IO_H)
    add_definitions(-DHAVE_IO_H=1)
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
    ${BOSON_INT_DIST_DIR}/include)

list(APPEND STRESSTESTS_SOURCES
    main.cc
    ../common/utils.cc
    node_stress_tests.cc
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

list(APPEND STRESSTESTS_DEPENDS
    CLI11
    sqlite
    CppUnit
    boson0
    libsodium)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(SYSTEM_LIBS pthread dl)
endif()

if(ENABLE_STATIC)
    set(LIBS boson-static ${LIBS})
endif()

add_executable(stresstest ${STRESSTESTS_SOURCES})
target_link_libraries(stresstest ${LIBS} ${SYSTEM_LIBS})
add_dependencies(stresstest ${STRESSTESTS_DEPENDS})

enable_testing()
add_test(TEST stresstest)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    install(TARGETS stresstest
        RUNTIME DESTINATION "bin"
        ARCHIVE DESTINATION "lib"
        LIBRARY DESTINATION "lib")
endif()

#install(FILES log_test.conf DESTINATION "bin")
