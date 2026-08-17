file(GLOB_RECURSE SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/* ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

add_library(DreamPlayer ${SRC})
add_library(Dream::Player ALIAS DreamPlayer)

set_target_properties(DreamPlayer PROPERTIES EXPORT_NAME Player)

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

if (WIN32)
	# Windows 平台复制 DLL
    install(FILES ${COPY_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})
elseif (UNIX AND NOT APPLE)
	set_target_properties(DreamPlayer PROPERTIES INSTALL_RPATH "$ORIGIN")
	install(FILES ${COPY_FILES} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

install(
    TARGETS DreamPlayer
    EXPORT DreamMediaNativeTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/DreamPlayer
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)