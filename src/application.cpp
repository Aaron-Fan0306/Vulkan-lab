#include <iostream>
#include <SDL3/SDL.h>
#include <volk.h>
#include "application.hpp"
#include "utilities/utilities.hpp"
#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

bool Application::initialize()
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        window = SDL_CreateWindow("vulkan test", m_width, m_height, SDL_WINDOW_VULKAN);
    }
    if (!initializeVulkan())
    {
        showError("failed to initialize vulkan");
        return false;
    }
    return true;
}

bool Application::initializeVulkan()
{
	if (!createVulkanInstance())
	{
		showError("Couldn't create a vulkan instance");
		return false;
	}

	if (!createSurface())
	{
		showError("Couldn't create window surface");
		return false;
	}

	if (physical_device = findPhysicalDevice(); !physical_device)
	{
		showError("Unable to find an appropriate physical device");
		return false;
	}

	if (!findGraphicsQueue())
	{
		showError("Unable to find a compatible graphics queue");
		return false;
	}

	if (!createDevice(physical_device))
	{
		showError("Couldn't create the logical GPU device");
		return false;
	}

    if (!initializeVMA())
    {
        showError("cannot initialize vma allocator");
        return false;
    }
	if (!createSwapchain(m_width, m_height))
	{
		showError("Unable to create swapchain");
		return false;
	}

    if (!createShaders())
    {
        showError("Unable to load shader");
        return false;
    }
    
    if (!createGraphicsPipelines())
    {
        showError("failed to create graphics pipeline");
        return false;
    }
    
    if (!syncResources())
    {
        showError("Unable to synce resources");
        return false;
    }
	
    if (!createCommandBuffers())
	{
		showError("Couldn't create command buffer objects");
		return false;
	}
	return true;
}

bool Application::createVulkanInstance()
{
    if (volkInitialize() != VK_SUCCESS)
    {
        showError("failed to init volk");
        return false;
    }
    VkApplicationInfo app_info
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan Lab",
        .apiVersion = vulkan_version
    };
    
    uint32_t instance_extensions_count = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&instance_extensions_count);
    std::vector<const char*> requested_layers
    {
        "VK_LAYER_KHRONOS_validation"
    };
    VkInstanceCreateInfo vk_instance_create_info
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(requested_layers.size()),
        .ppEnabledLayerNames = requested_layers.data(),
        .enabledExtensionCount = instance_extensions_count,
        .ppEnabledExtensionNames = extensions
    };
    if(vkCreateInstance(&vk_instance_create_info, nullptr, &vulkan_instance))
    {
        return false;
    }
    volkLoadInstance(vulkan_instance);
    return true;
}

bool Application::createSurface()
{
    if(!SDL_Vulkan_CreateSurface(window, vulkan_instance, nullptr, &surface))
    {
        showError("failed to create surface");
        return false;
    }
    return true;
}

VkPhysicalDevice Application::findPhysicalDevice() const
{
    uint32_t physical_device_count = 0;
    vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count, nullptr);
    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count, physical_devices.data());

    VkPhysicalDevice physical_device = nullptr;//this is so that we keep the one in application.hpp private

    if(physical_device_count)
    {
        physical_device = physical_devices[0];

        for (auto &pd : physical_devices)
        {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(pd, &properties);

            if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                physical_device = pd;
                break;
            }
        }
    }
//check for swapchain compatiblility
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, surface_formats.data());

    bool format_supported = false;
    for (const VkSurfaceFormatKHR &surf_format : surface_formats)
    {
        if (surf_format.format == swapchain_format)
        {
            format_supported = true;
            break;
        }
    }
    if(!format_supported)
    {
        showError("requested swapchain format is not support by surface");
    }
    return physical_device;
}

VkPipeline Application::createGraphicsPipelines()
{
    VkPipelineLayoutCreateInfo pipeline_layout_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0
    };
    if(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
    {
        showError("failed to create pipeline layout");
        return nullptr;
    }

    //configure the stader stages struct
    const char *entry_point = "main";
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages
    {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader,
            .pName = entry_point
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader,
            .pName = entry_point
        }
    };

    //vertex pulling, don't define vertex input details!
    VkPipelineVertexInputStateCreateInfo vertex_input_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    //input Assembly, we'll be drawing triangle lists
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    //depth/stencil configuration
    VkPipelineDepthStencilStateCreateInfo depth_stencil_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .stencilTestEnable = VK_FALSE
    };
	// dynamic rendering allows to set this up...dynamically, suprize huh
	// we still need this struct though
    VkPipelineViewportStateCreateInfo viewport_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount= 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr
    };

    //setting for the rasterizer, remember, we don't write rasterizers ourself, they're provided by the gpu company
    VkPipelineRasterizationStateCreateInfo rasterizer_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.f
    };
    // No multisampling
    VkPipelineMultisampleStateCreateInfo multi_sample_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };
    //Alpha-blending (disabled for now), still need
    //attachment info and write mask
    VkPipelineColorBlendAttachmentState attachment_state
    {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |VK_COLOR_COMPONENT_A_BIT
    };
    VkPipelineColorBlendStateCreateInfo blend_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment_state
    };
    std::vector<VkDynamicState> dynamic_state
    {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_state.size()),
        .pDynamicStates = dynamic_state.data()
    };

    //structs required for dynamic rendering
    VkPipelineRenderingCreateInfo render_info
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchain_format,
        .depthAttachmentFormat = depth_format
    };

    //create pipeline finally
    VkGraphicsPipelineCreateInfo pipeline_info
    {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &render_info,
        .stageCount = static_cast<uint32_t>(shader_stages.size()),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pViewportState = &viewport_info,
        .pRasterizationState = &rasterizer_info,
        .pMultisampleState = &multi_sample_info,
        .pDepthStencilState = &depth_stencil_info,
        .pColorBlendState = &blend_info,
        .pDynamicState = &dynamic_state_info,
        .layout = pipeline_layout,
        .renderPass = VK_NULL_HANDLE
    };
    if(vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS)
    {
        showError("cannot create the graphics pipeline");
        return nullptr;
    }
    return pipeline;
}

bool Application::findGraphicsQueue()
{
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties2> queue_family_properties(queue_family_count);
    for (auto &properties : queue_family_properties)
    {
        properties.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
    }
    vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_family_count, queue_family_properties.data());

    for (int i = 0; i < queue_family_properties.size(); ++i)// queue_family_properteis.size gives the elements of the vector
    {
        VkBool32 has_presentation_support = false;

        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &has_presentation_support);

        const VkQueueFamilyProperties2 &properties = queue_family_properties[i];
        if (properties.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT && has_presentation_support)// checks for graphics and presentation support
        {
            graphics_queue_family_index = i;
        }
    }
    return true;
}

bool Application::createDevice(VkPhysicalDevice physical_dev)
{
    float queue_priority = 1.0f;
    
    VkDeviceQueueCreateInfo graphics_queue_info
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphics_queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    VkPhysicalDeviceVulkan14Features supported_features14{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES, .pNext = nullptr};
	VkPhysicalDeviceVulkan13Features supported_features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &supported_features14 };
	VkPhysicalDeviceVulkan12Features supported_features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &supported_features13 };
	VkPhysicalDeviceFeatures2 supported_features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &supported_features12 };
    vkGetPhysicalDeviceFeatures2(physical_dev, &supported_features);
// order is important because pNext is a pointer to an object that's suppose to exist
    if (!supported_features13.dynamicRendering || !supported_features13.synchronization2 || !supported_features12.timelineSemaphore)
    {
        showError("device cannot support one of the following extension(versions): dynamic rendering 1.3, synchronization 1.3, timeline semaphore 1.2");
        return false;
    }
	VkPhysicalDeviceVulkan14Features features14
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
		.pNext = nullptr,
	};
	VkPhysicalDeviceVulkan13Features features13
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &features14,
		.synchronization2 = VK_TRUE,
		.dynamicRendering = VK_TRUE,
	};
	VkPhysicalDeviceVulkan12Features features12
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &features13,
		.timelineSemaphore = VK_TRUE
	};
    VkPhysicalDeviceFeatures2 features { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &features12 };

    const std::vector<const char*> device_extensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo device_create_info
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &graphics_queue_info,
        .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = nullptr
    };

    if (vkCreateDevice(physical_dev, &device_create_info, nullptr, &device) != VK_SUCCESS)
    {
        showError("Failed to create device");
        return false;
    }

    volkLoadDevice(device);
    vkGetDeviceQueue(device, graphics_queue_family_index, 0, &graphics_queue);
    
    if (!graphics_queue)
    {
        showError("cannot find/get graphics queue");
        return false;
    }
    return true;
}

bool Application::createSwapchain(uint32_t width, uint32_t height)
{
    swapchain_width = width;
    swapchain_height = height;

    VkSurfaceCapabilitiesKHR surface_capability{};
    if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capability) != VK_SUCCESS)
    {
        showError("Failed to get the surface capability");
        return false;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surface_capability.minImageCount,
        .imageFormat = swapchain_format,
        .imageColorSpace =VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageExtent{.width = swapchain_width, .height = swapchain_height},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR
    };
    if(vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain) != VK_SUCCESS)
    {
        showError("failed to create swapchain");
        return false;
    }
    uint32_t swapchain_image_count = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);
    swapchain_images.resize(swapchain_image_count);//set the elements of swapchain images to the amount of image needed
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());
    swapchain_image_views.resize(swapchain_image_count);

    for(size_t i = 0; i < swapchain_images.size(); ++i)
    {
        VkImageViewCreateInfo image_view_create_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain_format,
            .subresourceRange
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        };
        if (vkCreateImageView(device, &image_view_create_info, nullptr, &swapchain_image_views[i]) !=  VK_SUCCESS)
        {
            showError("failed to create swapchain image view");
            return false;
        }
    }

    render_complete_semaphores.resize(swapchain_images.size());
    //remember that this is a vector to sync frames so semaphores don't cross different frame and mix up the rendering

    for (VkSemaphore &semaphore : render_complete_semaphores)
    {
        VkSemaphoreCreateInfo semaphore_create_info { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        if(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphore) != VK_SUCCESS)
        {
            showError("Faild to create render semaphore");
            return false;
        }
    }
    //create depth image
    VkImageCreateInfo depth_image_create_info
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = depth_format,
		.extent{.width = swapchain_width, .height = swapchain_height, .depth = 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VmaAllocationCreateInfo allocation_info
    {
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    if (vmaCreateImage(vma_allocator, &depth_image_create_info, &allocation_info, &depth_image, &depth_image_allocation, nullptr) != VK_SUCCESS)
    {
        showError("failed to create depth image");
        return false;
    }

    VkImageViewCreateInfo image_view_create_info
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = depth_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = depth_format,
		.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1}
    };
    if (vkCreateImageView(device, &image_view_create_info, nullptr, &depth_image_view) != VK_SUCCESS)
    {
        showError("failed to create depth image view");
        return false;
    }

    return true;
}

bool Application::createCommandBuffers()
{   
    //modifies the frame_resource list directly by using the reference(&) symbol
    for (FrameResources &resources : frame_resources)
    {
        VkCommandPoolCreateInfo command_pool_create_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = graphics_queue_family_index
        };
        if(vkCreateCommandPool(device, &command_pool_create_info, nullptr, &resources.command_pool))
        {
            showError("failed to create command pool");
            return false;
        }

        VkCommandBufferAllocateInfo command_allocate_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = resources.command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };
        if(vkAllocateCommandBuffers(device, &command_allocate_info, &resources.command_buffer))
        {
            showError("failed to create command buffer");
        }
    }
    return true;
}

bool Application::syncResources()
{
    VkSemaphoreTypeCreateInfo timeline_semaphore_create_info
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = max_frames_in_flight
    };
    VkSemaphoreCreateInfo semaphore_create_info
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &timeline_semaphore_create_info
    };
    if (vkCreateSemaphore(device, &semaphore_create_info, nullptr, &timeline_semaphore))
    {
        showError("could not create the timeline semaphore(at function 'syncResources()')");
        return false;
    }
    for(FrameResources &resources : frame_resources)
    {
        VkSemaphoreCreateInfo semaphore_create_info { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        if (vkCreateSemaphore(device, &semaphore_create_info, nullptr, &resources.image_acquired_semaphore))
        {
            showError("failed to create image accquired semaphore");
            return false;
        }
    }
    return true;
}

bool Application::destroySwapchain()
{
   //no reference or ptr bc the view type is already a pointer
    for (VkImageView targeted_image_view : swapchain_image_views)
    {
        vkDestroyImageView(device, targeted_image_view, nullptr);
    }
    swapchain_image_views.clear();
    /*the loop and clear function does different things!
    loop for GPU and clear for CPU because clear does not free the actual space of the views.
    yes reference because view type is not a pointer*/
    for(VkSemaphore &targeted_semaphore : render_complete_semaphores)
    {
        vkDestroySemaphore(device, targeted_semaphore, nullptr);
    }
    if (swapchain)// on success it returns non zero so if becomes true
    {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        //swapchain has to go ka-boom with the image view :)
    }
    //this also ka-boom, but no function for that so we do it ourself:)
    if(depth_image_view)
    {
        vkDestroyImageView(device, depth_image_view, nullptr);
        vmaDestroyImage(vma_allocator, depth_image, depth_image_allocation);
        depth_image_view = nullptr;
    }
    return false;
}

bool Application::createShaders()
{
    if(vertex_shader = createShaderModule("shader.vertex", shaderc_vertex_shader); !vertex_shader)
        return false;
    if (fragment_shader = createShaderModule("shader.fragment", shaderc_fragment_shader) ; !fragment_shader)
        return false;
    return true;
}

VkShaderModule Application::createShaderModule(const std::string &file_name, shaderc_shader_kind kind) const
{
    const std::string shader_path = "shaders/" + file_name;
    const std::string src = readTextFile(shader_path);
    if(src.empty())
    {
        showError("could not find provided shader file path, Provided Path: " + shader_path);
    }
    
    //compile the shader to SPIR-V
    std::cout << "Compiling shader" << shader_path << "\n";
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
    options.SetTargetSpirv(shaderc_spirv_version_1_6);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    shaderc::CompilationResult result = compiler.CompileGlslToSpv(src, kind, file_name.c_str(), options);

    if(result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::cerr << "Shader Compilation Error: " << result.GetErrorMessage() << "\n";
        return nullptr;
    }
    std::vector<uint32_t> spir_v = { result.cbegin(), result.cend() };

    //pass spir-v to vulkan and create the shader module
    VkShaderModuleCreateInfo shader_module_info
    {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spir_v.size() * sizeof(uint32_t),
        .pCode = spir_v.data()
    };
    VkShaderModule shader_module = nullptr;
    if(vkCreateShaderModule(device, &shader_module_info, nullptr, &shader_module))
    {
        showError("failed to create shader module" + shader_path);
        return nullptr;
    }
    return shader_module;
}

void Application::render()
{
    if (swapchain_recreate)
    {
        vkDeviceWaitIdle(device);
        destroySwapchain();
        createSwapchain(m_width, m_height);
        swapchain_recreate = false;
    }
    frame_resources_index = frame_resources_index % max_frames_in_flight;
    const uint64_t signal_value = ++next_signal_value;
    const uint64_t wait_value = signal_value - max_frames_in_flight;
    VkSemaphoreWaitInfo wait_timeline_info
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = &timeline_semaphore,
        .pValues = &wait_value
    };
    vkWaitSemaphores(device, &wait_timeline_info, UINT64_MAX);

    FrameResources &resource = frame_resources[frame_resources_index];
    vkResetCommandPool(device, resource.command_pool, 0);

    VkSemaphore image_acquire_semaphore = frame_resources[frame_resources_index].image_acquired_semaphore;

    uint32_t image_index = 0;
    VkResult accquire_result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_acquire_semaphore, VK_NULL_HANDLE, &image_index);

    if (accquire_result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        swapchain_recreate = true;
        return;
    }
    else if (accquire_result == VK_SUBOPTIMAL_KHR)
    {
        swapchain_recreate = true;
        return; 
    }

    VkCommandBufferBeginInfo command_begin_info
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(resource.command_buffer, &command_begin_info);
/*remember that the layout barrier prepares the necessary settings and info for the next image,
and so it does not block anything specific, simply called a barrier because it prevents specific actions
from happening before something is done*/
    std::vector<VkImageMemoryBarrier2> layout_barriers
    {
        //color and depth image layout barrier.
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .image = swapchain_images[image_index],
            .subresourceRange
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        },
 {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .image = swapchain_images[image_index],
            .subresourceRange
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        },
    };
    VkDependencyInfo depth_info
    {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = static_cast<uint32_t>(layout_barriers.size()),
        .pImageMemoryBarriers = layout_barriers.data()
    };
   vkCmdPipelineBarrier2(resource.command_buffer, &depth_info);
   
   //set up attachemtns color and depth, start dynamic rendering.

   VkRenderingAttachmentInfo color_attachment_info
   {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = swapchain_image_views[image_index],
    .imageLayout =  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .clearValue{.color{0.01f, 0.01f, 0.01f, 1}}
   };
   VkRenderingAttachmentInfo depth_attachment_info
   {
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = depth_image_view,
    .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .clearValue{.depthStencil{1.0f, 0}}//second one is a uint32_t so not a float
   };
   VkRenderingInfo rendering_info{
       .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
       .renderArea
       {
            .offset{.x = 0, .y = 0},
            .extent{.width = swapchain_width, .height = swapchain_height}
       },
       .layerCount = 1,
       .colorAttachmentCount = 1,
       .pColorAttachments = &color_attachment_info,
       .pDepthAttachment = &depth_attachment_info,
   };

   //BEGIN DYNAMIC RENDERING!!!!!!!!!!!!!!!!!!!!!!! :) FINALLY!!!!!!!!!
   vkCmdBeginRendering(resource.command_buffer, &rendering_info);
   {
        VkViewport viewport
        {
            .x = 0, .y = 0,
            .width = static_cast<float>(swapchain_width),
            .height = static_cast<float>(swapchain_height)
        };
        vkCmdSetViewport(resource.command_buffer, 0, 1, &viewport);
        VkRect2D scissor
        {
            .offset{.x = 0, .y =0},
            .extent{.width = swapchain_width, .height = swapchain_height}
        };
        vkCmdSetScissor(resource.command_buffer, 0, 1, &scissor);

        //DRAW THE TRIANGLE!!!!!!!!!! OMG THIS IS IT!!!
        vkCmdBindPipeline(resource.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdDraw(resource.command_buffer, 3, 1, 0, 0);
   }
   //END DYNAMIC RENDERING
   vkCmdEndRendering(resource.command_buffer);
   //transition the image 
   VkImageMemoryBarrier2 present_layput_barrier
   {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    .image = swapchain_images[image_index],
    .subresourceRange
    {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    }
   };
   VkDependencyInfo present_depth_info
   {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &present_layput_barrier
    };
   
    vkCmdPipelineBarrier2(resource.command_buffer, &present_depth_info);
   vkEndCommandBuffer(resource.command_buffer);

   //ensure swapchain image is actually valible to start color output
   VkSemaphoreSubmitInfo image_acquire_wait_info
   {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = image_acquire_semaphore,
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
   };
   //signal that the image is ready to be presented
   std::vector<VkSemaphoreSubmitInfo> semaphore_signals
   {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = render_complete_semaphores[image_index],
            .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = timeline_semaphore,
            .value = signal_value,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
        }
   };
   VkCommandBufferSubmitInfo command_submit_info
   {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = resource.command_buffer
   };
   VkSubmitInfo2 submit_info
   {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &image_acquire_wait_info,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &command_submit_info,
        .signalSemaphoreInfoCount = static_cast<uint32_t>(semaphore_signals.size()),
        .pSignalSemaphoreInfos = semaphore_signals.data()
   };
   vkQueueSubmit2(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);

   //PRESENT THE IMAGE!!!!!!!!
   VkPresentInfoKHR  present_info
   {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &render_complete_semaphores[image_index],
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &image_index,
        .pResults = nullptr
   };

   vkQueuePresentKHR(graphics_queue, &present_info);
}

void Application::run()
{
    is_running = true;
    while(is_running)
    {
        SDL_Event event_msg { 0 };
        SDL_PollEvent(&event_msg);
        switch (event_msg.type)
        {
            case SDL_EVENT_QUIT:
                is_running = false;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                m_width = event_msg.window.data1;
                m_height = event_msg.window.data2;
                break;
            default:
                break;
        }
        render();
    }
}

int Application::shutDown()
{
    vkDeviceWaitIdle(device);

    //frame / sync object clean up
    if (timeline_semaphore)
    {
        vkDestroySemaphore(device, timeline_semaphore, nullptr);
    }
    for (FrameResources &resources : frame_resources)
    {
        vkDestroySemaphore(device, resources.image_acquired_semaphore, nullptr);
        vkDestroyCommandPool(device, resources.command_pool, nullptr);
    }
    //pipeline clean up
    if (pipeline_layout)
    {
        vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    }
    if (pipeline)
    {
        vkDestroyPipeline(device, pipeline, nullptr);
    }

    //shader clean ups

    if (vertex_shader)
    {
        vkDestroyShaderModule(device, vertex_shader, nullptr);
    }
    if (fragment_shader)
    {
        vkDestroyShaderModule(device, fragment_shader, nullptr);
    }

    //clean swapchain
    destroySwapchain();

    //VMA clean up
    if (vma_allocator)
    {
        vmaDestroyAllocator(vma_allocator);
    }
    //clean up vulkan
    if (surface)
    {
        vkDestroySurfaceKHR(vulkan_instance, surface, nullptr);
    }
    if (device)
    {
        vkDestroyDevice(device, nullptr);
    }
    if(vulkan_instance)
    {
        vkDestroyInstance(vulkan_instance, nullptr);
    }
    volkFinalize();
    std::cout << "destroying window" << std::endl;
    SDL_DestroyWindow(window);
    return 0;
}

bool Application::initializeVMA()
{
    VmaVulkanFunctions vma_function_info{};
    VmaAllocatorCreateInfo vma_allocator_info
    {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physical_device,
        .device = device,
        .pVulkanFunctions = &vma_function_info,
        .instance = vulkan_instance,
        .vulkanApiVersion = vulkan_version
    };

    //vma can important directly from volk
    vmaImportVulkanFunctionsFromVolk(&vma_allocator_info, &vma_function_info);

    if(vmaCreateAllocator(&vma_allocator_info, &vma_allocator) != VK_SUCCESS)
    {
        showError("Could not create allocator with volk");
        return false;
    }
    return true;
}

void Application::showError(const std::string &error_message) const
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "error", error_message.c_str(), window);
}