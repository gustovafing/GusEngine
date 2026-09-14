cmake_minimum_required(VERSION 3.25)

project(GusEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_image.h")
find_package(imgui CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)

set(SOURCES
        core/globals.cpp

        core/renderer/vkUtils.cpp
        core/renderer/vkAllocator.cpp
        core/renderer/renderer.cpp

        core/types/object.cpp
        core/types/variant_type.cpp
        core/types/type_registry.cpp
        core/types/resource.cpp

        core/engine.cpp

        filesystem/engine_io.cpp
        filesystem/resource_loader.cpp

        project/resources/image.cpp
        project/resources/shader.cpp

        utils/logger.cpp
        utils/unique_id.cpp

        main.cpp
)

add_executable(GusEngine
        ${SOURCES}
)

target_include_directories(GusEngine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        "${STB_INCLUDE_DIRS}"
)

target_link_libraries(GusEngine PRIVATE
        Vulkan::Vulkan
        glfw
        glm::glm
        imgui::imgui
        vk-bootstrap::vk-bootstrap
        glslang::glslang
        glslang::SPIRV
        glslang::glslang-default-resource-limits
)

if (MSVC)
    target_compile_options(GusEngine PRIVATE
            /W3
            /sdl
            /permissive-
    )

    target_link_options(GusEngine PRIVATE
            /IGNORE:4099
    )
endif()

target_compile_definitions(GusEngine PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        _CONSOLE
        GLM_FORCE_PURE
)

if (MSVC)
    target_compile_options(GusEngine PRIVATE
            $<$<CONFIG:Release>:/GL>
    )

    target_link_options(GusEngine PRIVATE
            $<$<CONFIG:Release>:/LTCG>
    )
endif()


set_target_properties(GusEngine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(GusEngine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)