find_package(Qt6 COMPONENTS OpenGL Core Multimedia Quick QuickControls2 Gui REQUIRED)
qt_standard_project_setup(REQUIRES 6.8)

find_package(Dream COMPONENTS Player REQUIRED)

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
get_resource_paths(FONT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resource/fonts/*)
set_source_files_properties(resource/PlayerTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

file(COPY resource/shader DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${COPY_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

qt_add_executable(playerApp ${SRC})

qt_add_qml_module(playerApp
        URI Dream.play
        VERSION 1.0
        RESOURCE_PREFIX "/qt/qml"
        QML_FILES
            ${QMLFILES}
        RESOURCES
            ${IMAGE_FILES}
            ${FONT_FILES}
            resource/shader/video.vert
            resource/shader/video.frag
)

if(WIN32)
    set_target_properties(playerApp PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN TRUE)
endif()

target_include_directories(playerApp
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(playerApp
        PRIVATE Dream::Player
        PRIVATE Qt6::OpenGL Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::Gui Qt6::Core
)

install(TARGETS playerApp
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
