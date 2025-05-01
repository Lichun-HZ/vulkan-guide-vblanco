//> includes
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_initializers.h>
#include <vk_types.h>

//bootstrap library
#include "VkBootstrap.h"

#include <chrono>
#include <thread>

VulkanEngine* loadedEngine = nullptr;
constexpr bool bUseValidationLayers = true;

VulkanEngine& VulkanEngine::Get() { return *loadedEngine; }
void VulkanEngine::init()
{
    // only one engine initialization is allowed with the application.
    assert(loadedEngine == nullptr);
    loadedEngine = this;

    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        _windowExtent.width,
        _windowExtent.height,
        window_flags);
    
    init_vulkan();
    init_swapchain();
    init_commands();
    init_sync_structures();
    
    // everything went fine
    _isInitialized = true;
}

void VulkanEngine::cleanup()
{
    if (_isInitialized)
    {
        // initialization order was SDL Window -> Instance -> Surface -> Device -> Swapchain,
        // we are doing exactly the opposite order for destruction.

        // 先调用会报错，用于测试validation layer是否有效
        //vkDestroyDevice(_device, nullptr);
        
        // 按照初始化的逆顺序，进行Destroy
        destroy_swapchain();

        vkDestroySurfaceKHR(_instance, _surface, nullptr);

        // 销毁device，VkPhysicalDevice不能被销毁，因为它不是Vulkan Resource, 而是系统GPU的一个Handle。
        vkDestroyDevice(_device, nullptr);
		
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
        
        SDL_DestroyWindow(_window);
    }

    // clear engine pointer
    loadedEngine = nullptr;
}

void VulkanEngine::draw()
{
    // nothing yet
}

void VulkanEngine::run()
{
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT)
                bQuit = true;

            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    stop_rendering = true;
                }
                if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                    stop_rendering = false;
                }
            }
        }

        // do not draw if we are minimized
        if (stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        draw();
    }
}

void VulkanEngine::init_vulkan()
{
    vkb::InstanceBuilder builder;

    //make the vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Example vulkan Application")
        .request_validation_layers(bUseValidationLayers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();

    vkb::Instance vkb_instance = inst_ret.value();

    // grab the instance
    _instance = vkb_instance.instance;
    _debug_messenger = vkb_instance.debug_messenger;

    // create vulkan surface
    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    // vulkan 1.3 feature
    VkPhysicalDeviceVulkan13Features vulkan13_features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    // dynamic rendering allows us to completely skip renderpasses/framebuffers
    vulkan13_features.dynamicRendering = true;
    // use a new upgraded version of the syncronization functions.
    vulkan13_features.synchronization2 = true;

    // vulkan 1.2 feature
    VkPhysicalDeviceVulkan12Features vulkan12_features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    // Buffer device adress will let us use GPU pointers without binding buffers
    vulkan12_features.bufferDeviceAddress = true;
    // descriptorIndexing gives us bindless textures.
    vulkan12_features.descriptorIndexing = true;

    // use vkbootstrap to select a gpu. 
    // We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
    vkb::PhysicalDeviceSelector selector{vkb_instance};
    vkb::PhysicalDevice physicalDevice = selector
        .set_minimum_version(1, 3)
        .set_required_features_13(vulkan13_features)
        .set_required_features_12(vulkan12_features)
        .set_surface(_surface)
        .select()
        .value();

    // create the final vulkan device
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle used in the rest of a vulkan application
    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;
}

void VulkanEngine::init_swapchain()
{
    create_swapchain(_windowExtent.width, _windowExtent.height);
}

void VulkanEngine::init_commands()
{
}

void VulkanEngine::init_sync_structures()
{
}

// 当窗口大小发生变化时，我们需要rebuild swapchain，所以需要将create_swapchain的逻辑从init_swapchain分离出来
void VulkanEngine::create_swapchain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchain_builder{_chosenGPU, _device, _surface};
    _swapChainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkb_swapchain = swapchain_builder
        //.use_default_format_selection()
        .set_desired_format(VkSurfaceFormatKHR{.format = _swapChainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        // use vsync present mode
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    _swapChainExtent = vkb_swapchain.extent;

    // store swapchain and its related images
    _swapchain = vkb_swapchain.swapchain;
    _swapChainImages = vkb_swapchain.get_images().value();
    _swapChainImageViews = vkb_swapchain.get_image_views().value();
}

void VulkanEngine::destroy_swapchain()
{
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    // There is no need to destroy the Images in this specific case,
    // because the images are owned and destroyed with the swapchain.
    
    // destroy swapchain resources
    for (int i = 0; i < _swapChainImageViews.size(); ++i)
    {
        vkDestroyImageView(_device, _swapChainImageViews[i], nullptr);
    }
}
