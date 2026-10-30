#pragma once

#include <SDL3/SDL_vulkan.h>
#include <string>
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <shaderc/shaderc.hpp>

struct VmaAllocator_T;
typedef struct VmaAllocator_T* VmaAllocator;
struct VmaAllocation_T;
typedef struct VmaAllocation_T* VmaAllocation;
struct FrameResources
{
    VkCommandPool command_pool = nullptr;
    VkCommandBuffer command_buffer = nullptr;
    VkSemaphore image_acquired_semaphore = nullptr;
};
class Application
{
public:
    bool initialize();//SDL window & Vulkan stuff too :)
    bool initializeVulkan();//all the stuff related to vulkan
    void run();
    int shutDown();
private:
    bool initializeVMA();
    void showError(const std::string& error_msg) const;
    VkPhysicalDevice findPhysicalDevice() const;
    VkPipeline createGraphicsPipelines();
    bool createSurface();
    bool createDevice(VkPhysicalDevice physical_dev);//logical device
    bool createVulkanInstance();
    bool findGraphicsQueue();
    bool createSwapchain(uint32_t width, uint32_t height);
    bool createCommandBuffers();
    bool syncResources();
    bool destroySwapchain();
    bool createShaders();
    VkShaderModule createShaderModule(const std::string &file_name, shaderc_shader_kind kind) const;
    void render();
//---------------------------variables--------------------------
    //constexpr stuff
    constexpr static VkFormat swapchain_format { VK_FORMAT_B8G8R8A8_SRGB };
    constexpr static uint32_t vulkan_version { VK_API_VERSION_1_4 };
    constexpr static uint32_t max_frames_in_flight { 2 };
    constexpr static VkFormat depth_format { VK_FORMAT_D32_SFLOAT };
    //windowing
    SDL_Window *window = nullptr;
    uint32_t m_width = 1280;
    uint32_t m_height = 720;
    //vulkan core
    VkInstance vulkan_instance = nullptr;
    VkPhysicalDevice physical_device = nullptr;
    VkDevice device = nullptr;
    VkSurfaceKHR surface = nullptr;
    VmaAllocator vma_allocator = nullptr;
    //queue stuff
    uint32_t graphics_queue_family_index;
    VkQueue graphics_queue = nullptr;
    //swapchain stuff
    VkSwapchainKHR swapchain = nullptr;
    std::vector<VkImage> swapchain_images {};
    std::vector<VkImageView> swapchain_image_views {};
    std::vector<VkSemaphore> render_complete_semaphores {};
    bool swapchain_recreate = false;
    uint32_t swapchain_width = 0;
    uint32_t swapchain_height = 0;
    uint32_t frame_resources_index = 0;
    uint64_t next_signal_value = max_frames_in_flight + 1;
    VkImage depth_image = nullptr;
    VkImageView depth_image_view;
    VmaAllocation depth_image_allocation = nullptr;
    //command buffer related
    VkSemaphore timeline_semaphore = nullptr;
    std::array<FrameResources, max_frames_in_flight> frame_resources;
    //graphics pipeline
    VkPipelineLayout pipeline_layout = nullptr;
    VkPipeline pipeline = nullptr;
    //shaders related
    VkShaderModule vertex_shader = nullptr;
    VkShaderModule fragment_shader = nullptr;
    //others
    bool is_running = false;
};
