IF(WIN32)
	file(GLOB_RECURSE files "${SPINE_DEP_DIR}/*.dll")
	foreach(filename ${files})
		execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${filename} ${CMAKE_BINARY_DIR}/bin)
	endforeach()

	file(GLOB_RECURSE files "$ENV{Qt5_DIR}/bin/icudt*.dll")
	foreach(filename ${files})
		get_filename_component(filename ${filename} NAME)
		configure_file($ENV{Qt5_DIR}/bin/${filename} ${CMAKE_BINARY_DIR}/bin/${filename} COPYONLY)
	endforeach()

	file(GLOB_RECURSE files "$ENV{Qt5_DIR}/bin/icuin*.dll")
	foreach(filename ${files})
		get_filename_component(filename ${filename} NAME)
		configure_file($ENV{Qt5_DIR}/bin/${filename} ${CMAKE_BINARY_DIR}/bin/${filename} COPYONLY)
	endforeach()

	file(GLOB_RECURSE files "$ENV{Qt5_DIR}/bin/icuuc*.dll")
	foreach(filename ${files})
		get_filename_component(filename ${filename} NAME)
		configure_file($ENV{Qt5_DIR}/bin/${filename} ${CMAKE_BINARY_DIR}/bin/${filename} COPYONLY)
	endforeach()

	configure_file($ENV{Qt5_DIR}/bin/libEGL.dll ${CMAKE_BINARY_DIR}/bin/libEGL.dll COPYONLY)
	configure_file($ENV{Qt5_DIR}/bin/libGLESv2.dll ${CMAKE_BINARY_DIR}/bin/libGLESv2.dll COPYONLY)
	configure_file($ENV{Qt5_DIR}/plugins/platforms/qwindows.dll ${CMAKE_BINARY_DIR}/bin/plugins/platforms/qwindows.dll COPYONLY)

	configure_file(${CMAKE_SOURCE_DIR}/config/qt.conf ${CMAKE_BINARY_DIR}/bin/qt.conf COPYONLY)

	configure_file($ENV{Qt5_DIR}/bin/Qt5Concurrent.dll ${CMAKE_BINARY_DIR}/bin/Qt5Concurrent.dll COPYONLY)
	configure_file($ENV{Qt5_DIR}/bin/Qt5Core.dll ${CMAKE_BINARY_DIR}/bin/Qt5Core.dll COPYONLY)
	configure_file($ENV{Qt5_DIR}/bin/Qt5Gui.dll ${CMAKE_BINARY_DIR}/bin/Qt5Gui.dll COPYONLY)
		configure_file($ENV{Qt5_DIR}/plugins/imageformats/qico.dll ${CMAKE_BINARY_DIR}/bin/plugins/imageformats/qico.dll COPYONLY)
		configure_file($ENV{Qt5_DIR}/plugins/imageformats/qjpeg.dll ${CMAKE_BINARY_DIR}/bin/plugins/imageformats/qjpeg.dll COPYONLY)
		configure_file($ENV{Qt5_DIR}/bin/Qt5Network.dll ${CMAKE_BINARY_DIR}/bin/Qt5Network.dll COPYONLY)
		configure_file($ENV{Qt5_DIR}/bin/Qt5Svg.dll ${CMAKE_BINARY_DIR}/bin/Qt5Svg.dll COPYONLY)
		configure_file($ENV{Qt5_DIR}/plugins/imageformats/qsvg.dll ${CMAKE_BINARY_DIR}/bin/plugins/imageformats/qsvg.dll COPYONLY)
	configure_file($ENV{Qt5_DIR}/bin/Qt5Widgets.dll ${CMAKE_BINARY_DIR}/bin/Qt5Widgets.dll COPYONLY)
	IF(WIN32)
		configure_file($ENV{Qt5_DIR}/bin/Qt5WinExtras.dll ${CMAKE_BINARY_DIR}/bin/Qt5WinExtras.dll COPYONLY)
	ENDIF(WIN32)
ENDIF(WIN32)
IF(UNIX)
	file(GLOB_RECURSE files "${SPINE_DEP_DIR}/*.so*")
	foreach(filename ${files})
		get_filename_component(filename_pure ${filename} NAME)
		execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${filename} ${CMAKE_BINARY_DIR}/bin/${filename_pure})
	endforeach()
ENDIF(UNIX)