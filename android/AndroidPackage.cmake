# SPDX-License-Identifier: MIT
#
# Android packaging for femm_gui. Included from gui/CMakeLists.txt. The solver
# targets are defined later in the top-level CMakeLists.txt; only their output
# paths and names are needed here, and Qt finalizes the target at the end of
# the gui directory.
#
#  * The four solvers are built as executables named lib<name>.so and listed
#    as extra native libraries, so the APK installs them into the app's native
#    library directory -- the only place Android lets an app execute a file
#    from. gui/android_support.cpp points libfemm at that directory.
#  * The read-only data that the desktop build stages into bin/ (material
#    libraries, the in-process Python stdlib) plus the example models go into
#    the APK's assets under femm/. android_support.cpp extracts them into the
#    app's private storage on first start.

function(femm_android_package APP)
    set(_solvers ${ARGN})
    set(_pkg "${CMAKE_BINARY_DIR}/android-package")
    set(_assets "${_pkg}/assets/femm")

    # Files that override Qt's Android template (manifest, gradle, res/).
    file(COPY "${CMAKE_SOURCE_DIR}/android/package/" DESTINATION "${_pkg}")

    set(_extra_libs "")
    foreach(_s IN LISTS _solvers)
        list(APPEND _extra_libs "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib${_s}.so")
    endforeach()

    add_custom_target(femm_android_assets
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${_assets}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_assets}/app" "${_assets}/examples"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python" "${_assets}/app/python"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${CMAKE_SOURCE_DIR}/matlibs/matlib.dat"
                "${CMAKE_SOURCE_DIR}/matlibs/statlib.dat"
                "${CMAKE_SOURCE_DIR}/matlibs/heatlib.dat"
                "${CMAKE_SOURCE_DIR}/matlibs/condlib.dat"
                "${_assets}/app/"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_SOURCE_DIR}/examples" "${_assets}/examples"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_SOURCE_DIR}/android/scripts" "${_assets}/examples"
        COMMENT "Staging FEMM data into the Android assets")
    add_dependencies(femm_android_assets pybridge_python femm_pylib)
    add_dependencies(${APP} femm_android_assets ${_solvers})

    if(NOT FEMM_ANDROID_VERSION_CODE)
        set(FEMM_ANDROID_VERSION_CODE 1)
    endif()

    set_target_properties(${APP} PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${_pkg}"
        QT_ANDROID_EXTRA_LIBS "${_extra_libs}"
        QT_ANDROID_PACKAGE_NAME "io.github.firatturkerirci.femm"
        QT_ANDROID_APP_NAME "FEMM"
        QT_ANDROID_VERSION_NAME "${PROJECT_VERSION}"
        QT_ANDROID_VERSION_CODE "${FEMM_ANDROID_VERSION_CODE}"
        QT_ANDROID_TARGET_SDK_VERSION 34
        QT_ANDROID_MIN_SDK_VERSION 28)
    if(COMMAND qt_policy)
        qt_policy(SET QTP0002 NEW)
    endif()

endfunction()
