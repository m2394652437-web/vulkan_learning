cmake_minimum_required(VERSION 3.11.0)
project(LveEngine VERSION 0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# 收集源文件
file(GLOB_RECURSE SOURCES src/*.cpp)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME} 
    glfw
    Vulkan::Vulkan
    glm::glm
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

# 编译着色器
find_program(GLSL_VALIDATOR glslangValidator)
if(GLSL_VALIDATOR)
    # 创建输出目录
    set(SPIRV_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
    file(MAKE_DIRECTORY ${SPIRV_OUTPUT_DIR})
    
    # 查找所有着色器
    file(GLOB_RECURSE SHADER_FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag
    )
    
    # 为每个着色器生成编译命令
    foreach(SHADER ${SHADER_FILES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SPV_FILE ${SPIRV_OUTPUT_DIR}/${SHADER_NAME}.spv)
        
        add_custom_command(
            OUTPUT ${SPV_FILE}
            COMMAND ${GLSL_VALIDATOR} -V ${SHADER} -o ${SPV_FILE}
            DEPENDS ${SHADER}
            COMMENT "Compiling ${SHADER_NAME}"
        )
        list(APPEND SPV_FILES ${SPV_FILE})
    endforeach()
    
    # 创建着色器编译目标
    add_custom_target(Shaders DEPENDS ${SPV_FILES})
    add_dependencies(${PROJECT_NAME} Shaders)
else()
    message(WARNING "glslangValidator not found, shaders will not be compiled")
endif()

#设置工作目录为 build 目录
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
