// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

// We will have the deletion queue in multiple places, for multiple lifetimes of objects.
// One of them is on the engine class itself, and will be flushed when the engine gets destroyed.
// Global objects go into that one. We will also store one deletion queue for each frame in flight,
// which will allow us to delete objects next frame after they are used.

struct FrameData
{
	VkCommandPool	command_pool;
	VkCommandBuffer _mainCommandBuffer;

	VkSemaphore		_swapchainSemaphore, _renderSemaphore;
	VkFence			_renderFence;

	DeletionQueue _deletionQueue;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
public:
	VkInstance					_instance;			// Vulkan library handle
	VkDebugUtilsMessengerEXT	_debug_messenger;	// Vulkan debug output handle
	VkPhysicalDevice			_chosenGPU;			// GPU chosen as the default device
	VkDevice					_device;			// Vulkan device for commands
	VkSurfaceKHR				_surface;			// Vulkan window surface

	VkSwapchainKHR				_swapchain;
	VkFormat					_swapChainImageFormat;

	std::vector<VkImage>		_swapChainImages;
	std::vector<VkImageView>	_swapChainImageViews;
	VkExtent2D					_swapChainExtent;

	VkQueue						_graphicsQueue;
	uint32_t					_graphicsQueueFamily;

	FrameData					_frames[FRAME_OVERLAP];
	DeletionQueue				_mainDeletionQueue;
	VmaAllocator				_allocator;

	//draw resources
	AllocatedImage				_drawImage;
	VkExtent2D					_drawExtent;
	
	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP];}
	
	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	static VulkanEngine& Get();

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

private:
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();
	
	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();

	void draw_background(VkCommandBuffer cmd);
};
