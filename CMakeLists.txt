project(rime-msgpack)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PROJECT_SOURCE_DIR})

aux_source_directory(src msgpack_src)

add_library(rime-msgpack-objs OBJECT ${msgpack_src})
target_include_directories(rime-msgpack-objs PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
if(BUILD_SHARED_LIBS)
  set_target_properties(rime-msgpack-objs
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON)
endif()

set(plugin_name rime-msgpack PARENT_SCOPE)
set(plugin_objs $<TARGET_OBJECTS:rime-msgpack-objs> PARENT_SCOPE)
set(plugin_deps ${rime_library} PARENT_SCOPE)
set(plugin_modules "msgpack" PARENT_SCOPE)

# msvc doesn't export all symbols
if(NOT (WIN32 AND BUILD_SHARED_LIBS))
  add_executable(rime_msgpack_console "rime_msgpack_console.cc" $<TARGET_OBJECTS:rime-msgpack-objs>)
  target_link_libraries(rime_msgpack_console ${rime_library})
  target_include_directories(rime_msgpack_console PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/src)
endif()
