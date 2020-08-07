# -----------------------------------------------------------------------------
# Build executable
# -----------------------------------------------------------------------------
set(BUILD_TARGET "main")

set(SOURCE_FILES ${SOURCE_FILES} "main.cpp")
add_folder(SOURCE_FILES "ext")
add_folder(SOURCE_FILES "glad")
add_folder(SOURCE_FILES "core")

file(GLOB SHADER_FILES "shaders/*.vert" "shaders/*.frag" "shaders/*.comp")

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/ext)
include_directories(${GLFW3_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS})
add_executable(${BUILD_TARGET} ${SOURCE_FILES} ${SHADER_FILES})

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
    set_property(TARGET ${BUILD_TARGET} APPEND PROPERTY LINK_FLAGS "/DEBUG /PROFILE")
endif()

source_group("Source Files" FILES ${SOURCE_FILES})
source_group("shaders" FILES ${SHADER_FILES})

target_link_libraries(${BUILD_TARGET} ${CXX_FS_LIBRARY} ${GLFW3_LIBRARIES} ${GLFW3_STATIC_LIBRARIES})

# -----------------------------------------------------------------------------
# Copy shader programs
# -----------------------------------------------------------------------------
set(OUTPUT_SHADERS "")
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/shaders")

foreach (SHADER IN LISTS SHADER_FILES)
    get_filename_component(BASE_NAME ${SHADER} NAME)
    set(OUTPUT_SHADERS ${OUTPUT_SHADERS} ${SHADER_OUTPUT_DIR}/${BASE_NAME})
endforeach()

if (OUTPUT_SHADERS)
    add_custom_command(OUTPUT ${OUTPUT_SHADERS}
                       COMMAND ${CMAKE_COMMAND}
                       ARGS -E remove_directory "${SHADER_OUTPUT_DIR}"
                       COMMAND ${CMAKE_COMMAND}
                       ARGS -E make_directory  "${SHADER_OUTPUT_DIR}"
                       COMMAND ${CMAKE_COMMAND}
                       ARGS -E copy ${SHADER_FILES} "${SHADER_OUTPUT_DIR}"
                       DEPENDS ${SHADER_FILES})

    add_custom_target(COPY_SHADER_FILES ALL
                      SOURCES ${OUTPUT_SHADERS})

    add_dependencies(${BUILD_TARGET} COPY_SHADER_FILES)
endif()
