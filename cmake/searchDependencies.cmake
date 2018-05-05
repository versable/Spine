SET(CMAKE_LIBRARY_PATH ${SPINE_DEP_DIR})

#----------------------------------------------------
# clockUtils
#----------------------------------------------------

IF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/clockUtils/")
	execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-clockUtils.bat ${VS_TOOLCHAIN} ${VS_ARCH} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
ENDIF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/clockUtils/")
IF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/clockUtils/")
	execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-clockUtils.sh ${UNIX_COMPILER} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
ENDIF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/clockUtils/")
SET(LIBNAME "CLOCKUTILS")
SET(LIBHEADER "clockUtils/errors.h")
SET(CLOCKUTILS_ROOT ${SPINE_DEP_DIR}/clockUtils)
SET(CLOCKUTILS_COMPONENT ${CLOCKUTILS_COMPONENT} clock_compression)
IF(WITH_CLIENT)
	SET(CLOCKUTILS_COMPONENT ${CLOCKUTILS_COMPONENT} clock_log)
ENDIF(WITH_CLIENT)
SET(CLOCKUTILS_COMPONENT ${CLOCKUTILS_COMPONENT} clock_sockets)

find_package(EasyFind REQUIRED COMPONENTS ${CLOCKUTILS_COMPONENT})
include_directories(SYSTEM ${CLOCKUTILS_INCLUDE_DIR})

#----------------------------------------------------
# GMock
#----------------------------------------------------

IF(WITH_CLIENT)
	IF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/gmock/")
		execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-gmock.bat ${VS_TOOLCHAIN} ${VS_ARCH} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
	ENDIF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/gmock/")
	IF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/gmock/")
		execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-gmock.sh ${UNIX_COMPILER} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
	ENDIF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/gmock/")
	SET(LIBNAME "GTEST")
	SET(LIBHEADER "gtest/gtest.h")
	SET(GTEST_ROOT ${SPINE_DEP_DIR}/gmock)
	SET(GTEST_COMPONENT ${GTEST_COMPONENT} gtest)

	find_package(EasyFind REQUIRED COMPONENTS ${GTEST_COMPONENT})
	include_directories(SYSTEM ${GTEST_INCLUDE_DIR})
ENDIF(WITH_CLIENT)

#----------------------------------------------------
# MariaDB
#----------------------------------------------------

IF(WITH_SERVER)
	IF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/mariadb/")
		execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-mariadb.bat ${VS_TOOLCHAIN} ${VS_ARCH} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
	ENDIF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/mariadb/")
	IF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/mariadb/")
		execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-mariadb.sh ${UNIX_COMPILER} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
	ENDIF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/mariadb/")
	SET(LIBNAME "MARIADB")
	SET(LIBHEADER "mariadb/m_string.h")
	SET(MARIADB_ROOT ${SPINE_DEP_DIR}/mariadb)
	IF(WIN32)
		SET(MARIADB_COMPONENT ${MARIADB_COMPONENT} libmariadb)
	ELSEIF(UNIX)
		SET(MARIADB_COMPONENT ${MARIADB_COMPONENT} mariadb)
	ENDIF()

	find_package(EasyFind REQUIRED COMPONENTS ${MARIADB_COMPONENT})
	include_directories(SYSTEM ${MARIADB_INCLUDE_DIR})
	include_directories(SYSTEM ${MARIADB_INCLUDE_DIR}/mariadb)
ENDIF(WITH_SERVER)

#----------------------------------------------------
# OpenSSL
#----------------------------------------------------

IF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/openSSL/")
	execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-openSSL.bat ${VS_TOOLCHAIN} ${VS_ARCH} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
ENDIF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/openSSL/")
IF(UNIX AND NOT ANDROID)
	find_package(OpenSSL REQUIRED)
ELSEIF(WIN32)
	SET(LIBNAME "OPENSSL")
	SET(LIBHEADER "openssl/rsa.h")
	SET(OPENSSL_ROOT ${SPINE_DEP_DIR}/openSSL)
	find_package(EasyFind REQUIRED COMPONENTS libeay32 ssleay32)
ENDIF()
include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})
#----------------------------------------------------
# Qt
#----------------------------------------------------

IF(WITH_CLIENT)
	IF(UNIX AND NOT ANDROID)
		SET(ENV{Qt5_DIR} "")
	ENDIF()

	SET(QT_WEBENGINE_VERSION 5.6.0)

	# Test for supported Qt version
	find_program(QMAKE_EXECUTABLE NAMES qmake HINTS $ENV{Qt5_DIR} ENV QTDIR PATH_SUFFIXES bin)
	execute_process(COMMAND ${QMAKE_EXECUTABLE} -query QT_VERSION OUTPUT_VARIABLE QT_VERSION)

	SET(QT_SEARCH_MODULES "")

	IF(QT_VERSION LESS QT_WEBENGINE_VERSION)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} Concurrent)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} Core)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} Gui)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} LinguistTools)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} Network)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} Svg)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} Widgets)
	IF(WIN32)
			SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} WinExtras)
	ENDIF(WIN32)
	ELSE()
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} Concurrent)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} Core)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} Gui)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} LinguistTools)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} Network)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} Svg)
		SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} Widgets)
		IF(WIN32)
			SET(QT_SEARCH_MODULES ${QT_SEARCH_MODULES} WinExtras)
		ENDIF(WIN32)
	ENDIF()

	FIND_PACKAGE(Qt5 REQUIRED ${QT_SEARCH_MODULES} HINTS $ENV{Qt5_DIR})

	IF(QT_VERSION LESS QT_WEBENGINE_VERSION)
		INCLUDE_DIRECTORIES(${Qt5Concurrent_INCLUDE_DIRS})
		SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5Concurrent_LIBRARIES})
		INCLUDE_DIRECTORIES(${Qt5Core_INCLUDE_DIRS})
		SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5Core_LIBRARIES})
		INCLUDE_DIRECTORIES(${Qt5Gui_INCLUDE_DIRS})
		SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5Gui_LIBRARIES})
		INCLUDE_DIRECTORIES(${Qt5Network_INCLUDE_DIRS})
		SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5Network_LIBRARIES})
		INCLUDE_DIRECTORIES(${Qt5Svg_INCLUDE_DIRS})
		SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5Svg_LIBRARIES})
		INCLUDE_DIRECTORIES(${Qt5Widgets_INCLUDE_DIRS})
		SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5Widgets_LIBRARIES})
		IF(WIN32)
			INCLUDE_DIRECTORIES(${Qt5WinExtras_INCLUDE_DIRS})
			SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5WinExtras_LIBRARIES})
		ENDIF(WIN32)
	ELSE()
		INCLUDE_DIRECTORIES(${Qt5Concurrent_INCLUDE_DIRS})
		SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5Concurrent_LIBRARIES})
		INCLUDE_DIRECTORIES(SYSTEM ${Qt5Core_INCLUDE_DIRS})
		SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5Core_LIBRARIES})
		INCLUDE_DIRECTORIES(SYSTEM ${Qt5Gui_INCLUDE_DIRS})
		SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5Gui_LIBRARIES})
		INCLUDE_DIRECTORIES(SYSTEM ${Qt5Network_INCLUDE_DIRS})
		SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5Network_LIBRARIES})
		INCLUDE_DIRECTORIES(${Qt5Svg_INCLUDE_DIRS})
		SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5Svg_LIBRARIES})
		INCLUDE_DIRECTORIES(SYSTEM ${Qt5Widgets_INCLUDE_DIRS})
		SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5Widgets_LIBRARIES})
		IF(WIN32)
			INCLUDE_DIRECTORIES(${Qt5WinExtras_INCLUDE_DIRS})
			SET(QT_LIBRARIES ${QT_LIBRARIES} ${Qt5WinExtras_LIBRARIES})
		ENDIF(WIN32)
	ENDIF()
ENDIF(WITH_CLIENT)


#----------------------------------------------------
# SQLite
#----------------------------------------------------

IF(WITH_CLIENT)
	IF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/sqlite/")
		execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-sqlite.bat ${VS_TOOLCHAIN} ${VS_ARCH} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
	ENDIF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/sqlite/")
	IF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/sqlite/")
		execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-sqlite.sh ${UNIX_COMPILER} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
	ENDIF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/sqlite/")
	SET(LIBNAME "SQLITE3")
	SET(LIBHEADER "sqlite3.h")
	SET(SQLITE3_ROOT ${SPINE_DEP_DIR}/sqlite)
	find_package(EasyFind REQUIRED COMPONENTS sqlite3)
	include_directories(SYSTEM ${SQLITE3_INCLUDE_DIR})
ENDIF(WITH_CLIENT)

#----------------------------------------------------
# tinyxml
#----------------------------------------------------

IF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/tinyxml2/")
	execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-tinyxml.bat ${VS_TOOLCHAIN} ${VS_ARCH} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
ENDIF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/tinyxml2/")
IF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/tinyxml2/")
	execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-tinyxml.sh ${UNIX_COMPILER} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
ENDIF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/tinyxml2/")
SET(LIBNAME "TINYXML")
SET(LIBHEADER "tinyxml2.h")
SET(TINYXML_ROOT ${SPINE_DEP_DIR}/tinyxml2)
find_package(EasyFind REQUIRED COMPONENTS tinyxml2)
include_directories(SYSTEM ${TINYXML_INCLUDE_DIR})

#----------------------------------------------------
# zlib
#----------------------------------------------------

IF(WITH_CLIENT)
	IF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/zlib/")
		execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-zlib.bat ${VS_TOOLCHAIN} ${VS_ARCH} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
	ENDIF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/zlib/")
	IF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/zlib/")
		execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-zlib.sh ${UNIX_COMPILER} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
	ENDIF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/zlib/")
	SET(LIBNAME "ZLIB")
	SET(LIBHEADER "zlib.h")
	SET(ZLIB_ROOT ${SPINE_DEP_DIR}/zlib)
	IF(WIN32)
		SET(ZLIB_COMPONENT ${ZLIB_COMPONENT} zlibstatic)
	ELSE()
		SET(ZLIB_COMPONENT ${ZLIB_COMPONENT} z)
	ENDIF()

	find_package(EasyFind REQUIRED COMPONENTS ${ZLIB_COMPONENT})
	include_directories(SYSTEM ${ZLIB_INCLUDE_DIR})
ENDIF(WITH_CLIENT)

#----------------------------------------------------
# Boost
#----------------------------------------------------

IF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/boost/")
	execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-boost.bat ${VS_TOOLCHAIN} ${VS_ARCH} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
ENDIF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/boost/")
IF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/boost/")
	execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-boost.sh ${UNIX_COMPILER} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
ENDIF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/boost/")
SET(LIBNAME "BOOST")
SET(LIBHEADER "boost/thread.hpp")
SET(BOOST_ROOT ${SPINE_DEP_DIR}/boost)
SET(BOOST_COMPONENT ${BOOST_COMPONENT} boost_date_time)
IF(WITH_SERVER)
	SET(BOOST_COMPONENT ${BOOST_COMPONENT} boost_filesystem)
ENDIF(WITH_SERVER)
IF(WITH_CLIENT)
	SET(BOOST_COMPONENT ${BOOST_COMPONENT} boost_iostreams)
ENDIF(WITH_CLIENT)
SET(BOOST_COMPONENT ${BOOST_COMPONENT} boost_serialization)
SET(BOOST_COMPONENT ${BOOST_COMPONENT} boost_system)
SET(BOOST_COMPONENT ${BOOST_COMPONENT} boost_thread)
find_package(EasyFind REQUIRED COMPONENTS ${BOOST_COMPONENT})
include_directories(SYSTEM ${BOOST_INCLUDE_DIR})

#----------------------------------------------------
# Translator
#----------------------------------------------------

IF(WITH_CLIENT AND WITH_TRANSLATOR)
       IF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/translator/")
               execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-translator.bat ${VS_TOOLCHAIN} ${VS_ARCH} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
       ENDIF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/translator/")
       IF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/translator/")
               execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-translator.sh ${UNIX_COMPILER} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
       ENDIF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/translator/")
       SET(LIBNAME "TRANSLATOR")
       SET(LIBHEADER "translator/api/TranslatorAPI.h")
       SET(TRANSLATOR_ROOT ${SPINE_DEP_DIR}/translator)
       SET(TRANSLATOR_COMPONENT ${TRANSLATOR_COMPONENT} TranslatorAPI TranslatorCommon)

       find_package(EasyFind REQUIRED COMPONENTS ${TRANSLATOR_COMPONENT})
       include_directories(SYSTEM ${TRANSLATOR_INCLUDE_DIR})
ENDIF(WITH_CLIENT AND WITH_TRANSLATOR)

#----------------------------------------------------
# SimpleWebServer
#----------------------------------------------------

IF(WITH_CLIENT)
	IF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/SimpleWebServer/")
		execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-simplewebserver.bat ${VS_TOOLCHAIN} ${VS_ARCH} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
	ENDIF(WIN32 AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/SimpleWebServer/")
	IF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/SimpleWebServer/")
		execute_process(COMMAND ${CMAKE_SOURCE_DIR}/dependencies/build-simplewebserver.sh ${UNIX_COMPILER} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/dependencies)
	ENDIF(UNIX AND NOT ANDROID AND NOT EXISTS "${SPINE_DEP_DIR}/SimpleWebServer/")
	SET(LIBNAME "SIMPLEWEBSERVER")
	SET(LIBHEADER "SimpleWebServer/server_https.hpp")
	SET(SIMPLEWEBSERVER_ROOT ${SPINE_DEP_DIR}/SimpleWebServer)

	include_directories(SYSTEM ${SIMPLEWEBSERVER_ROOT}/include)
ENDIF(WITH_CLIENT)
