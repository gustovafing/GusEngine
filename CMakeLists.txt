cmake_minimum_required(VERSION 3.25)

project(GusEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

set(GLFW_ROOT "C:/Users/ofoxs/source/modules/glfw-3.4.bin.WIN64")
set(GLM_ROOT  "C:/Users/ofoxs/source/modules/glm-1.0.1")
set(STB_ROOT  "C:/Users/ofoxs/source/modules/stb")

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
        utils/uniqueId.cpp

        external/imGUI/imgui.cpp
        external/imGUI/imgui_demo.cpp
        external/imGUI/imgui_draw.cpp
        external/imGUI/imgui_tables.cpp
        external/imGUI/imgui_widgets.cpp
        external/imGUI/imgui_impl_glfw.cpp
        external/imGUI/imgui_impl_vulkan.cpp

        external/vkBootstrap/VkBootstrap.cpp

        main.cpp
)

add_executable(GusEngine
        ${SOURCES}
)

target_include_directories(GusEngine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}

        "${GLFW_ROOT}/include"
        "${GLM_ROOT}"
        "${STB_ROOT}"

        "$ENV{VULKAN_SDK}/Include"
        "${CMAKE_CURRENT_SOURCE_DIR}/external"
)

target_link_directories(GusEngine PRIVATE
        "${GLFW_ROOT}/lib-vc2022"
)

target_link_libraries(GusEngine PRIVATE
        Vulkan::Vulkan
        glfw3
)

if (MSVC)
    target_compile_options(GusEngine PRIVATE
            /W3
            /sdl
            /permissive-
    )

    # Equivalent to the project's /IGNORE:4099
    target_link_options(GusEngine PRIVATE
            /IGNORE:4099
    )
endif()

target_compile_definitions(GusEngine PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        _CONSOLE
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