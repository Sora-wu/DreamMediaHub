find_package(Qt6 COMPONENTS Core Multimedia Quick Gui REQUIRED)
qt_standard_project_setup(REQUIRES 6.8)

message(STATUS ${CMAKE_PREFIX_PATH})
find_package(Dream COMPONENTS Recorder REQUIRED)

function(get_resource_paths out_var base_path)
    file(GLOB ABS_ASSETS ${base_path})
    foreach(ABS_ASSET ${ABS_ASSETS})
        file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${ABS_ASSET})
        list(APPEND out_var ${REL_PATH})
    endforeach()
    set(${out_var} PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)
get_resource_paths(QMLFILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/*.qml)
get_resource_paths(IMAGE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/image/*)

file(COPY ${COPY_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

qt_add_executable(recorderApp ${SRC})

qt_add_qml_module(recorderApp
        URI Dream.recorder
        VERSION 1.0
        RESOURCE_PREFIX "/qt/qml"
        QML_FILES
            ${QMLFILES}
        RESOURCES
            ${IMAGE_FILES}
)

if(WIN32)
    find_package(Qt6 COMPONENTS QuickControls2 REQUIRED)
    target_link_libraries(recorderApp PRIVATE Qt6::QuickControls2)
    set_target_properties(recorderApp PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN TRUE)
endif()

target_include_directories(recorderApp
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(recorderApp
        PRIVATE Dream::Recorder
        PRIVATE Qt6::Quick Qt6::Multimedia Qt6::Gui Qt6::Core
)

install(TARGETS recorderApp
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)