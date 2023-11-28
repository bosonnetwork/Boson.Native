project(boson-core CXX)

include(ProjectDefaults)
include(CheckIncludeFile)

add_definitions(-DSODIUM_STATIC)

if(WIN32)
    add_definitions(
        -D_WINSOCK_DEPRECATED_NO_WARNINGS
        -D_CRT_SECURE_NO_WARNINGS
        -DWIN32_LEAN_AND_MEAN)
endif()

if (ENABLE_BOSON_DEVELOPMENT)
    add_definitions(-DBOSON_DEVELOPMENT)
endif()

if (ENABLE_BOSON_CRAWLER)
    add_definitions(-DBOSON_CRAWLER)
endif()

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

list(APPEND BOSON_SOURCES
    core/utils/addr.cc
    core/utils/blob.cc
    core/utils/log.cc
    core/utils/socket_address.cc
    core/utils/json_to_any.cc
    core/crypto/base58.cc
    core/crypto/crypto_box.cc
    core/crypto/signature.cc
    core/crypto/shasum.cc
    core/crypto/random.cc
    core/crypto/hex.cc
    core/messages/message.cc
    core/messages/message_key.cc
    core/messages/announce_peer_request.cc
    core/messages/error_message.cc
    core/messages/find_peer_response.cc
    core/messages/find_value_request.cc
    core/messages/find_value_response.cc
    core/messages/lookup_request.cc
    core/messages/lookup_response.cc
    core/messages/store_value_request.cc
    core/task/task.cc
    core/task/task_manager.cc
    core/task/lookup_task.cc
    core/task/value_lookup.cc
    core/task/value_announce.cc
    core/task/peer_announce.cc
    core/task/peer_lookup.cc
    core/task/node_lookup.cc
    core/task/ping_refresh_task.cc
    core/task/closest_candidates.cc
    core/serializers.cc
    core/id.cc
    core/node_info.cc
    core/peer_info.cc
    core/value.cc
    core/version.cc
    core/prefix.cc
    core/crypto_context.cc
    core/kbucket_entry.cc
    core/kclosest_nodes.cc
    core/kbucket.cc
    core/routing_table.cc
    core/dht.cc
    core/node.cc
    core/token_manager.cc
    core/rpccall.cc
    core/rpcserver.cc
    core/rpcstatistics.cc
    core/sqlite_storage.cc
    core/default_configuration.cc
    core/constants.cc
)

list(APPEND BOSON_HEADERS
    ${INCLUDE_DIR}/boson/addon.h
    ${INCLUDE_DIR}/boson/blob.h
    ${INCLUDE_DIR}/boson/def.h
    ${INCLUDE_DIR}/boson/types.h
    ${INCLUDE_DIR}/boson/crypto_box.h
    ${INCLUDE_DIR}/boson/signature.h
    ${INCLUDE_DIR}/boson/socket_address.h
    ${INCLUDE_DIR}/boson/prefix.h
    ${INCLUDE_DIR}/boson/id.h
    ${INCLUDE_DIR}/boson/configuration.h
    ${INCLUDE_DIR}/boson/default_configuration.h
    ${INCLUDE_DIR}/boson/lookup_option.h
    ${INCLUDE_DIR}/boson/node_info.h
    ${INCLUDE_DIR}/boson/peer_info.h
    ${INCLUDE_DIR}/boson/value.h
    ${INCLUDE_DIR}/boson/version.h
    ${INCLUDE_DIR}/boson/node_status.h
    ${INCLUDE_DIR}/boson/connection_status.h
    ${INCLUDE_DIR}/boson/node.h
    ${INCLUDE_DIR}/boson/network.h
    ${INCLUDE_DIR}/boson/result.h
)

include_directories(
    ${INCLUDE_DIR}
    ./core
    ./addons/activeproxy
    )

list(APPEND BOSON_DEPENDS
    sqlite
    libsodium
    spdlog
    spdlog-setup
    nlohmann
    libutf8proc
    libuv
)

set(LIBS
    utf8proc
    sqlite3
    uv_a)

if(WIN32)
    set(LIBS
        ${LIBS}
        libsodium.lib
        Ws2_32
        crypt32
        Rpcrt4
        iphlpapi
        Shlwapi)
else()
    set(LIBS
        ${LIBS}
        sodium)
endif()

if(ENABLE_ADDONS)
    list(APPEND BOSON_SOURCES
        addons/activeproxy/activeproxy.cc
        addons/activeproxy/connection.cc
        addons/addons.cc
    )

    list(APPEND BOSON_DEPENDS
        libuv)

    set(LIBS
        ${LIBS}
        uv_a)

if(WIN32)
    set(LIBS
        ${LIBS}
        Userenv
        )
endif()

endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(SYSTEM_LIBS pthread dl)
endif()

add_custom_target(boson0)

if (CMAKE_PREFIX_PATH)
    add_custom_target(phoney0 COMMAND echo "phony target")
    set(BOSON_DEPENDS phoney0)
    set(CMAKE_FIND_ROOT_PATH  ${CMAKE_PREFIX_PATH})
else()
    set(CMAKE_FIND_ROOT_PATH ${PROJECT_INT_DIST_DIR})
endif()

if(ENABLE_STATIC)

    add_library(boson-static STATIC ${BOSON_SOURCES} ${BOSON_HEADERS})
    add_dependencies(boson-static ${BOSON_DEPENDS})
    add_dependencies(boson0 boson-static)

    target_compile_definitions(boson-static PRIVATE BOSON_BUILD BOSON_STATIC)
    set_target_properties(boson-static PROPERTIES OUTPUT_NAME boson${STATIC_LIB_DECORATOR})

    target_include_directories(boson-static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_FIND_ROOT_PATH}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(boson-static INTERFACE ${LIBS} ${SYSTEM_LIBS})

    install(TARGETS boson-static
        RUNTIME DESTINATION "bin"
        ARCHIVE DESTINATION "lib"
        LIBRARY DESTINATION "lib")
endif()

if(ENABLE_SHARED)
    add_library(boson-shared SHARED ${BOSON_SOURCES} ${BOSON_HEADERS})
    add_dependencies(boson-shared ${BOSON_DEPENDS})
    add_dependencies(boson0 boson-shared)

    target_compile_definitions(boson-shared PRIVATE BOSON_BUILD BOSON_DYNAMIC)
    set_target_properties(boson-shared PROPERTIES
        OUTPUT_NAME boson${SHARED_LIB_DECORATOR}
        VERSION ${CMAKE_PROJECT_VERSION}
        SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR})

    target_include_directories(boson-shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_FIND_ROOT_PATH}/include>
        $<INSTALL_INTERFACE:include>
    )

    target_link_directories(boson-shared
        PRIVATE ${CMAKE_FIND_ROOT_PATH}/lib)

    if(NOT APPLE)
        target_link_libraries(boson-shared
            PRIVATE ${LIBS} ${SYSTEM_LIBS})
    else()
        target_link_libraries(boson-shared
            PRIVATE ${LIBS} ${SYSTEM_LIBS}
            SYSTEM "-framework CoreFoundation" "-framework Security")
    endif()

    install(TARGETS boson-shared
        EXPORT BosonConfig
        RUNTIME DESTINATION "bin"
        ARCHIVE DESTINATION "lib"
        LIBRARY DESTINATION "lib")
endif()

install(FILES ${boson.hEADERS} DESTINATION "include/boson")
install(FILES "${INCLUDE_DIR}/boson.h" DESTINATION "include")
