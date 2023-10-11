project(nlohmann)

include(ProjectDefaults)
include(ExternalProject)
include(ExternalCMakeArgs)

ExternalProject_Add(
    nlohmann

    PREFIX ${PROJECT_DEPS_BUILD_PREFIX}
    URL "https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.tar.gz"
    URL_HASH SHA256=d69f9deb6a75e2580465c6c4c5111b89c4dc2fa94e3a85fcd2ffcd9a143d9273
    DOWNLOAD_NAME "json-3.11.2.tar.gz"
    DOWNLOAD_DIR ${PROJECT_DEPS_TARBALL_DIR}
    DOWNLOAD_NO_PROGRESS 1

    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_INT_DIST_DIR} -DJSON_BuildTests=OFF
)
