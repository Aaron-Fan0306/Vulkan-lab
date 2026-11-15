#pragma once

#include <vector>
#include <array>
#include <filesystem>

#define VK_NO_PROTOTYPE
#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>

#include <shaderc/shaderc.hpp>
#include <stb_image.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <vma/vk_mem_alloc.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>

#include "resources.hpp"

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

struct GPUImage
{
    VkImage image = nullptr;
    VkImageView image_view;
    VmaAllocation allocation = nullptr;
};

struct GPUBuffer
{
    VkBuffer vk_buffer = nullptr;
    uint64_t device_address = 0;
    VmaAllocation allocation = nullptr;
};

struct RederIteM 
{
    glm::mat4 wvp;
    glm::mat4 world_matrix;
    uint32_t material_index;
};

struct Material
{
    glm::vec4 base_color = glm::vec4(1.f, 1.f, 1.f, 1.f);
    uint32_t texture_index = 0;
};

struct Texture
{
    uint32_t image_id = 0;
    uint32_t sampler_id = 0;
};

struct FrameConstants
{
    uint64_t vertex_buffer_address = 0;
    uint64_t material_buffer_address = 0;
    uint64_t render_items_address = 0;
};

class Application
{
public:
    bool initialize();//SDL window & Vulkan stuff too :)
    bool initializeVulkan();//all the stuff related to vulkan
    bool loadData();
    void run();
    int shutDown();
private:
    bool initializeVMA();
    VkCommandBuffer startTransientCommandBuffer();
    void submitTransientCommandBuffer(VkCommandBuffer command_buffer);
    bool createVulkanInstance();
    bool createSurface();
    VkPhysicalDevice findPhysicalDevice() const;
    VkPipeline createGraphicsPipelines();
    bool createDevice(VkPhysicalDevice physical_dev);//logical device
    bool createSwapchain(uint32_t width, uint32_t height);
    bool findGraphicsQueue();
    bool createCommandBuffers();
    bool syncResources();
    bool destroySwapchain();
    bool createShaders();
    VkShaderModule createShaderModule(const std::string &file_name, shaderc_shader_kind kind) const;
    std::pair<uint32_t, GPUBuffer> createImage(VkCommandBuffer command_buffer, unsigned char * image_data, uint32_t width, uint32_t height, int channels);
    std::vector<uint32_t> upLoadImage(const std::vector<Image> &images);
    //helper functions
    uint32_t addBuffer(const GPUBuffer &buffer);
    GPUBuffer createBuffer(VkBufferUsageFlags buffer_usage, size_t byte_size, bool mappable, VmaMemoryUsage memory_usage);
    void mapCopyBufferData(const GPUBuffer &buffer, size_t buffer_offset, void *data, size_t byte_size);

    void render();
    
    void showError(const std::string& error_msg) const;
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
    VkCommandPool command_pool = nullptr;
    VkSemaphore timeline_semaphore = nullptr;
    std::array<FrameResources, max_frames_in_flight> frame_resources;
    //graphics pipeline
    VkPipelineLayout pipeline_layout = nullptr;
    VkPipeline pipeline = nullptr;
    //shaders related
    VkShaderModule vertex_shader = nullptr;
    VkShaderModule fragment_shader = nullptr;
    //cpu resources
    std::vector<Mesh> m_meshes;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    size_t m_vertex_offset = 0;
    size_t m_index_offset = 0;
    //gpu resources
    uint32_t m_white_pixel_image_id;
    uint32_t m_vertex_buffer_id;
    uint32_t m_index_buffer_id;
    uint32_t m_mat_buffer_id;
    std::vector<GPUImage> m_images;
    std::vector<VkSampler> m_samplers;
    std::vector<Vertex> m_vertices;
    std::vector<GPUBuffer> m_buffers;
    std::vector<Material> m_material;
    //descriptors
    VkDescriptorSetLayout m_global_DSL_layout = nullptr;
    VkDescriptorSet m_global_descriptor_set = nullptr;
    VkDescriptorPool m_descriptor_pool = nullptr;
    //others
    bool is_running = false;
};
