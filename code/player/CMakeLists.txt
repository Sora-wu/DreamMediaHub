find_package(Qt6 COMPONENTS OpenGLWidgets Core Quick Gui REQUIRED)
qt_standard_project_setup(REQUIRES 6.5)

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

qt_add_executable(playerApp ${SRC})

qt_add_qml_module(playerApp
        URI Dream.play
        RESOURCE_PREFIX "/"
        QML_FILES
            ${QMLFILES}
        RESOURCES
            ${IMAGE_FILES}
)

target_include_directories(playerApp
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(playerApp
        PRIVATE Qt6::OpenGLWidgets Qt6::Quick Qt6::Gui Qt6::Core
)

install(TARGETS playerApp
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)