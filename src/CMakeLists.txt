set(EXECUTABLE biomes)

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
else()
  add_compile_options(-Wall -O2 -g)
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)
find_package(SFML 2 COMPONENTS system window graphics REQUIRED)

set(SOURCES
  main.cpp
  simplexnoise.hpp simplexnoise.cpp
)

add_executable(${EXECUTABLE} WIN32 MACOSX_BUNDLE ${SOURCES})

set_property(TARGET ${EXECUTABLE} PROPERTY CXX_STANDARD 14)

include_directories(${SFML_INCLUDE_DIR})

target_link_libraries(${EXECUTABLE} ${SFML_LIBRARIES})

add_custom_target(run COMMAND ${EXECUTABLE} DEPENDS ${EXECUTABLE} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
