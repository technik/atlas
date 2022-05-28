################################################################################
# DX12 test renderer
################################################################################
cmake_minimum_required (VERSION 3.20)
project(atlas)

# Global options
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 20)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

# Clean Windows headers
add_definitions(-DWIN32_LEAN_AND_MEAN -DWIN32_EXTRA_LEAN -DNOMINMAX)

find_package(D3D12 REQUIRED)

macro(GroupSources curdir)
    file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir}
${PROJECT_SOURCE_DIR}/${curdir}/*)
    foreach(child ${children})
        if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
            GroupSources(${curdir}/${child})
        else()
            string(REPLACE "/" "\\" groupname ${curdir})
        # I would like to call the src root folder in a different name, only in visual studio (not mandatory requirement)
            string(REPLACE "src" "Common" groupname ${groupname})
            source_group(${groupname} FILES ${PROJECT_SOURCE_DIR}/${curdir}/${child})
          endif()
   endforeach()
endmacro()

###################################################################################################
macro(compile_HLSL_flags _SOURCE SOURCE_LIST OUTPUT_LIST _PROFILE)
    get_filename_component(FILE_NAME ${_SOURCE} NAME)
    set(OUTPUT ${FILE_NAME}.cso)
    set(ASM ${FILE_NAME}.asm)

    LIST(APPEND ${SOURCE_LIST} ${_SOURCE})
    LIST(APPEND ${OUTPUT_LIST} ${OUTPUT})

    set(_COMMAND dxc -E main -Fc ${ASM} -Zi -Fd ${FILE_NAME}.pdb -Fo ${OUTPUT} -I "../src/shaders/" -T ${_PROFILE} ${_SOURCE})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/bin/${OUTPUT}
        COMMAND echo ${_COMMAND}
        COMMAND ${_COMMAND}
        MAIN_DEPENDENCY ${_SOURCE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
        )
endmacro()

###################################################################################################
# Shader specializations
###################################################################################################
macro(addVertexShader SRC SOURCE_LIST OUTPUT_LIST)
  compile_HLSL_flags(${SRC} ${SOURCE_LIST} ${OUTPUT_LIST} vs_6_5)
endmacro()

#####################################################################################
macro(addPixelShader SRC SOURCE_LIST OUTPUT_LIST)
  compile_HLSL_flags(${SRC} ${SOURCE_LIST} ${OUTPUT_LIST} ps_6_5)
endmacro()

#####################################################################################
macro(addComputeShader SRC SOURCE_LIST OUTPUT_LIST)
  compile_HLSL_flags(${SRC} -T cs_6_5 ${SOURCE_LIST} ${OUTPUT_LIST})
endmacro()

file(GLOB_RECURSE HLSL_HEADER_FILES "src/shaders/*.hlsl" "shaders/*.h")
file(GLOB_RECURSE VERTEX_SOURCE_FILES "src/shaders/*.vshader")
file(GLOB_RECURSE PIXEL_SOURCE_FILES "src/shaders/*.pshader")
file(GLOB_RECURSE COMPUTE_SOURCE_FILES "src/shaders/*.cshader")

set(HLSL_SOURCES)
set(HLSL_OUTPUT)
foreach(SHADER ${VERTEX_SOURCE_FILES})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    addVertexShader(${SHADER} HLSL_SOURCES HLSL_OUTPUT)
endforeach()
foreach(SHADER ${PIXEL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    addPixelShader(${SHADER} HLSL_SOURCES HLSL_OUTPUT)
endforeach()
foreach(SHADER ${COMPUTE_SOURCE_FILES})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    addComputeShader(${SHADER} HLSL_SOURCES HLSL_OUTPUT)
endforeach()

# Main target
file(GLOB_RECURSE VIEWER_SRC "src/*.cpp" "src/*.h")
GroupSources(atlas)
add_executable(atlas ${VIEWER_SRC} ${HLSL_SOURCES})
target_include_directories(atlas PUBLIC
    include
    include/dx12headers
    include/json/include
    ${D3D12_INCLUDE_DIRS}
    src)
target_link_libraries(atlas ${D3D12_LIBRARIES})