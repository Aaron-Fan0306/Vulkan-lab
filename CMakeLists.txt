cmake_minimum_required(VERSION 3.24)

project(VulkanLab VERSION 0.1.0 LANGUAGES C CXX
)

# --------------------------------------------------
# Installed dependencies
# --------------------------------------------------

find_package(Vulkan REQUIRED)
find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(glm CONFIG REQUIRED)

# --------------------------------------------------
# Volk
# --------------------------------------------------

include(FetchContent)

# Volk itself must be compiled with the required platform definition.
set(
    VOLK_STATIC_DEFINES
    VK_USE_PLATFORM_WIN32_KHR
    CACHE STRING
    "Platform definitions used while compiling Volk"
    FORCE
)

FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG 1.4.304
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(volk)

# --------------------------------------------------
# VulkanLab executable
# --------------------------------------------------

add_executable(VulkanLab
    src/main.cpp
    src/application.cpp
    includes/application.hpp
    src/utilities/utilities.cpp
)

target_compile_features(VulkanLab
    PRIVATE
        cxx_std_20
)

set_target_properties(VulkanLab PROPERTIES
    CXX_EXTENSIONS OFF
)

target_include_directories(VulkanLab
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/includes"
)

target_compile_definitions(VulkanLab
    PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        VK_USE_PLATFORM_WIN32_KHR
        VK_NO_PROTOTYPES
)

target_link_libraries(VulkanLab
    PRIVATE
        Vulkan::Headers
        volk::volk
        SDL3::SDL3
        glm::glm
        Vulkan::Vulkan
        "C:/VulkanSDK/1.4.350.0/Lib/shaderc_sharedd.lib"
)

if(MINGW)
    target_compile_options(VulkanLab
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

# Copy SDL3.dll beside VulkanLab.exe after every build.
if(WIN32)
    add_custom_command(
        TARGET VulkanLab
        POST_BUILD
        COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3-shared>"
            "$<TARGET_FILE_DIR:VulkanLab>"
        VERBATIM
    )
endif()