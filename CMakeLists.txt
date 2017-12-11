cmake_minimum_required(VERSION 3.10.0)
project(rediswraps)

# version info
set(REDISWRAPS_MAJOR_VERSION 0)
set(REDISWRAPS_MINOR_VERSION 1)
set(REDISWRAPS_PATCH_VERSION 0)
set(REDISWRAPS_VERSION
${REDISWRAPS_MAJOR_VERSION}.${REDISWRAPS_MINOR_VERSION}.${REDISWRAPS_PATCH_VERSION})

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Paths
#   to FindMyLibrary.cmake files
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
#   installation
#FIXME get hard-coded /usr/local out of here:
set(INSTALL_INCLUDE_DIR /usr/local/include)
set(INSTALL_LIB_DIR     /usr/local/lib)

#   sources
set(SOURCE_FILES
  src/utils.cc
  src/response.cc
  src/connection.cc
)
#   headers
set(HEADER_FILES
  include/${PROJECT_NAME}/rediswraps.hh
  include/${PROJECT_NAME}/constants.hh
  include/${PROJECT_NAME}/utils.hh
  include/${PROJECT_NAME}/response.hh
  include/${PROJECT_NAME}/connection.hh
)

# make the build directory if it doesn't exist
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/build)

# compiler options/flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wfatal-errors -pedantic-errors")

if(NOT DEFINED BOOST_ROOT            AND
  NOT DEFINED  ENV{BOOST_ROOT}       AND
  NOT DEFINED  Boost_INCLUDEDIR      AND
  NOT DEFINED  ENV{Boost_INCLUDEDIR} AND
  NOT DEFINED  Boost_LIBRARYDIR      AND
  NOT DEFINED  ENV{Boost_LIBRARYDIR})

  if (APPLE)
    set(BOOST_ROOT "/usr/local/Cellar/boost/1.65.1")
  elseif (WIN32)
    #TODO
    #set(Boost_INCLUDEDIR "C:/???")
    #set(Boost_LIBRARYDIR "C:/???")
  endif()
endif()

# acquire dependencies
#   Boost headers
#set(Boost_DEBUG 1)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED)

if(NOT Boost_FOUND)
  message(STATUS "Boost libraries not found.  Downloading them now.")

  add_library(Boost INTERFACE IMPORTED)
  set_property(TARGET Boost PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES
    ${Boost_INCLUDE_DIR})

  install(DIRECTORY ${Boost_INCLUDE_DIR}/boost)
endif()


#   hiredis
find_package(hiredis)

if(NOT HIREDIS_FOUND)
  message(STATUS "Hiredis library not found.  Downloading and making it now.")

  set(HIREDIS_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/hiredis)

  execute_process(COMMAND git submodule update --init -- ${HIREDIS_INCLUDE_DIR})
  execute_process(COMMAND make -C ${HIREDIS_INCLUDE_DIR} clean)
  execute_process(COMMAND make -C ${HIREDIS_INCLUDE_DIR} install)
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE hiredis)
include_directories(include)

set_property(TARGET ${PROJECT_NAME}
  APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
  $<BUILD_INTERFACE:include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)

file(MAKE_DIRECTORY ${INSTALL_INCLUDE_DIR})

install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_LIB_DIR})
install(FILES ${HEADER_FILES} DESTINATION ${INSTALL_INCLUDE_DIR})

