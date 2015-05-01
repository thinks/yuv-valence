CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)
PROJECT(yuv-valence)

FIND_PACKAGE(OpenGL)

# Default to release build
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF()
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Project Compiler Flags
#ADD_DEFINITIONS(-Wall -Wextra)

SET(fstudio_HEADERS
  Camera.hpp
  Color.hpp
  Light.hpp
  Material.hpp
)

SET(fstudio_SOURCES
  main.cpp
)

MESSAGE(STATUS "GLEW_PATH: $ENV{GLEW_PATH}")
MESSAGE(STATUS "GLFW_PATH: $ENV{GLFW_PATH}")
MESSAGE(STATUS "NDJINN_PATH: $ENV{NDJINN_PATH}")
MESSAGE(STATUS "THX_PATH: $ENV{THX_PATH}")

ADD_SUBDIRECTORY($ENV{GLFW_PATH} lib)

INCLUDE_DIRECTORIES(
  $ENV{GLEW_PATH}/include
  $ENV{GLFW_PATH}/include
  $ENV{NDJINN_PATH}/include
  $ENV{THX_PATH}/include)

LINK_DIRECTORIES(
  $ENV{GLEW_PATH}/lib)

ADD_DEFINITIONS(-DGLEW_STATIC)

ADD_EXECUTABLE(yuv-valence
  ${fstudio_SOURCES}
  ${fstudio_HEADERS})

TARGET_LINK_LIBRARIES(yuv-valence
  ${OPENGL_LIBRARIES}
  ${GLFW_LIBRARIES}
  glfw
  glew32s)

#SET(fstudio_SHADERS
#  shaders/phong.vs
#  shaders/phong.gs
#  shaders/phong.fs)

#INSTALL(FILES ${fstudio_SHADERS} DESTINATION shaders)

add_custom_command(TARGET yuv-valence PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:yuv-valence>/shaders)
