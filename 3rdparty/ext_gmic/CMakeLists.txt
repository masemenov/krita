SET(PREFIX_ext_gmic "${EXTPREFIX}" )

# Download the gmic sources
ExternalProject_Add( ext_gmic_base
    DOWNLOAD_DIR ${EXTERNALS_DOWNLOAD_DIR}
    URL https://gmic.eu/files/source/gmic_2.4.2.tar.gz
    URL_MD5 a43389984672136ab04e0a170ed233b7

    SOURCE_DIR gmic

    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""

    BUILD_IN_SOURCE 1
)

# Download and build gmic-qt
# FIXME: Forcing CMAKE_BUILD_TYPE to Release
ExternalProject_Add( ext_gmic_qt
    DOWNLOAD_DIR ${EXTERNALS_DOWNLOAD_DIR}
    URL https://github.com/c-koi/gmic-qt/archive/v.2.4.2.tar.gz
    URL_MD5 09d952786980d76df8feb2aca0167b17
    PATCH_COMMAND ${PATCH_COMMAND} -p1 -i ${CMAKE_CURRENT_SOURCE_DIR}/desktop_icon.diff

    SOURCE_DIR gmic-qt
    INSTALL_DIR ${PREFIX_ext_gmic}

    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PREFIX_ext_gmic} -DGMIC_QT_HOST=krita -DCMAKE_BUILD_TYPE=Release ${GLOBAL_PROFILE}

    UPDATE_COMMAND ""

    DEPENDS ext_gmic_base
)

add_custom_target(ext_gmic)
add_dependencies(ext_gmic ext_gmic_qt)
