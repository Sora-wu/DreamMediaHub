file(GLOB_RECURSE SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/* ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

add_library(DreamRecorder ${SRC})
add_library(Dream::Recorder ALIAS DreamRecorder)

set_target_properties(DreamRecorder PROPERTIES EXPORT_NAME Recorder)

target_include_directories(DreamRecorder
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${FFMPEG_ROOT}/include
        include
)
target_link_directories(DreamRecorder PRIVATE ${FFMPEG_ROOT}/lib)
target_link_libraries(DreamRecorder PRIVATE ${FFMPEG_LIBS})

if (WIN32)
    # Windows 平台复制 DLL
    install(FILES ${COPY_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})
elseif (UNIX AND NOT APPLE)
    set_target_properties(DreamRecorder PROPERTIES INSTALL_RPATH "$ORIGIN")
    install(FILES ${COPY_FILES} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

install(
    TARGETS DreamRecorder
    EXPORT DreamMediaNativeTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/DreamRecorder
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)