cmake_minimum_required(VERSION 3.25) 
project(Infection CXX)               
set(CMAKE_CXX_STANDARD 17)            
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include("${CMAKE_SOURCE_DIR}/../../addons/prism/cmake/prism.cmake" REQUIRED)

# Add source/header files to project
file(GLOB_RECURSE SOURCES ../*.cpp SOURCES ../*.h SOURCES ../*.rc)
list(FILTER SOURCES EXCLUDE REGEX ".*web.*")
list(FILTER SOURCES EXCLUDE REGEX ".*/build/.*")

add_link_options(/NODEFAULTLIB:libcmt.lib)
add_link_options(/IGNORE:4099,4286,4098)
add_link_options(/OPT:REF)
add_link_options(/OPT:ICF)

# Define exe
add_executable(Infection ${SOURCES})

# Link with libraries
target_link_libraries(Infection prism)
target_link_libraries(Infection SDL2)
target_link_libraries(Infection SDL2main)
target_link_libraries(Infection SDL2_image)
target_link_libraries(Infection SDL2_mixer)
target_link_libraries(Infection SDL2_ttf)
target_link_libraries(Infection enet)
target_link_libraries(Infection freetype)
target_link_libraries(Infection glew)
target_link_libraries(Infection curl)
target_link_libraries(Infection libjpeg)
target_link_libraries(Infection libpng)
target_link_libraries(Infection libwebp)
target_link_libraries(Infection zstd)
target_link_libraries(Infection zlib)
target_link_libraries(Infection crypt32)
target_link_libraries(Infection wldap32)
target_link_libraries(Infection ws2_32)
target_link_libraries(Infection winmm)
target_link_libraries(Infection OpenGL32)
target_link_libraries(Infection version)

# Define preprocessor definitions
target_compile_definitions(Infection PUBLIC UNICODE)
target_compile_definitions(Infection PUBLIC _UNICODE)

set_property(TARGET Infection PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_compile_options(Infection PRIVATE /Gy)

set_target_properties(Infection PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../assets)
set_target_properties(Infection PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/../assets)

# Copy over DLLs
file(GLOB DLLS "${CMAKE_SOURCE_DIR}/../../addons/prism/windows/vs17/DLL/*.dll")
foreach(DLL ${DLLS})
  add_custom_command(TARGET Infection POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${DLL}
    $<TARGET_FILE_DIR:Infection>)
endforeach()

# Optimize for release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(Infection PRIVATE /O2)
endif()