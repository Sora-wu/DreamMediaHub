file(GLOB_RECURSE SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/* ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

add_library(DreamPlayer ${SRC})
add_library(Dream::Player ALIAS DreamPlayer)
target_include_directories(DreamPlayer 
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	PRIVATE
		${FFMPEG_ROOT}/include 
		include
)
target_link_directories(DreamPlayer PRIVATE ${FFMPEG_ROOT}/lib)
target_link_libraries(DreamPlayer PRIVATE ${FFMPEG_LIBS})

# Windows 平台复制 DLL
if (WIN32)
    add_custom_command(TARGET DreamPlayer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${COPY_FILES}
        $<TARGET_FILE_DIR:DreamPlayer>
    )
    install(FILES ${COPY_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(
    TARGETS DreamPlayer
    EXPORT DreamPlayerTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/DreamPlayer
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT DreamPlayerTargets
    FILE DreamPlayerConfig.cmake
    NAMESPACE Dream::
    DESTINATION ${Dream_INSTALL_CMAKEDIR}
)