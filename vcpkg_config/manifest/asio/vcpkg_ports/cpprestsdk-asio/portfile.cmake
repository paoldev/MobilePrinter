vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO paoldev/cpprestsdk
    REF f8f3c6895c0e52c6ea9ccf0e8ab064cbcc2aeaf2 
    SHA512 120c03c4d9d106465ba323cec0105291c0f3fc9987a64a386f128433835920212f6e2e4f9b2e6bec7c3924d15b34c6bf9ac47a724f75d430570e94c46e578988
    HEAD_REF windows-asio-fix
    PATCHES fix-find-openssl.patch
)

set(OPTIONS)
if(NOT VCPKG_TARGET_IS_UWP)
    SET(WEBSOCKETPP_PATH "${CURRENT_INSTALLED_DIR}/share/websocketpp")
    list(APPEND OPTIONS
        -DWEBSOCKETPP_CONFIG=${WEBSOCKETPP_PATH}
        -DWEBSOCKETPP_CONFIG_VERSION=${WEBSOCKETPP_PATH})
endif()

if("windows-asio" IN_LIST FEATURES)
    list(APPEND OPTIONS
        -DCPPREST_HTTP_CLIENT_IMPL=asio
		-DCPPREST_HTTP_LISTENER_IMPL=asio)
endif()

vcpkg_check_features(
    OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    INVERTED_FEATURES
      brotli CPPREST_EXCLUDE_BROTLI
      compression CPPREST_EXCLUDE_COMPRESSION
      websockets CPPREST_EXCLUDE_WEBSOCKETS
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}/Release
    PREFER_NINJA
    OPTIONS
        ${OPTIONS}
        ${FEATURE_OPTIONS}
        -DBUILD_TESTS=OFF
        -DBUILD_SAMPLES=OFF
        -DCPPREST_EXPORT_DIR=share/cpprestsdk-asio
        -DWERROR=OFF
        -DPKG_CONFIG_EXECUTABLE=FALSE
    OPTIONS_DEBUG
        -DCPPREST_INSTALL_HEADERS=OFF
)

vcpkg_install_cmake()

vcpkg_copy_pdbs()

vcpkg_fixup_cmake_targets(CONFIG_PATH lib/share/${PORT})
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/lib/share ${CURRENT_PACKAGES_DIR}/lib/share)

if (VCPKG_LIBRARY_LINKAGE STREQUAL static)
    vcpkg_replace_string(${CURRENT_PACKAGES_DIR}/include/cpprest/details/cpprest_compat.h
        "#ifdef _NO_ASYNCRTIMP" "#if 1")
endif()

file(INSTALL ${SOURCE_PATH}/license.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
