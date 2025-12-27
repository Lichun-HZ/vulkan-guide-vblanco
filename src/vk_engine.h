// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <imgui_impl_vulkan.h>
#include <vk_types.h>

#include "vk_descriptors.h"
#include "vk_initializers.h"
#include "vk_loader.h"

// We will have the deletion queue in multiple places, for multiple lifetimes of objects.
// One of them is on the engine class itself, and will be flushed when the engine gets destroyed.
// Global objects go into that one. We will also store one deletion queue for each frame in flight,
// which will allow us to delete objects next frame after they are used.

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct FrameData
{
	VkCommandPool	command_pool;
	VkCommandBuffer _mainCommandBuffer;

	VkSemaphore		_swapchainSemaphore, _renderSemaphore;
	VkFence			_renderFence;

	DeletionQueue _deletionQueue;
};

struct ComputeEffect {
	const char* name;

	VkPipeline pipeline;
	VkPipelineLayout layout;

	ComputePushConstants data;
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
	AllocatedImage				_depthImage;
	VkExtent2D					_drawExtent;
	
	DescriptorAllocator			globalDescriptorAllocator;
	VkDescriptorSet				_drawImageDescriptors;
	VkDescriptorSetLayout		_drawImageDescriptorLayout;

	VkPipeline					_gradientPipeline;
	VkPipelineLayout			_gradientPipelineLayout;

	// immediate submit structures
	VkFence						_immFence;
	VkCommandBuffer				_immCommandBuffer;
	VkCommandPool				_immCommandPool;
	
	VkPipelineLayout			_trianglePipelineLayout;
	VkPipeline					_trianglePipeline;
	
	VkPipelineLayout			_meshPipelineLayout;
	VkPipeline					_meshPipeline;
	GPUMeshBuffers				_rectangle;

	std::vector<std::shared_ptr<MeshAsset>> _testMeshes;
	
	void init_triangle_pipeline();
	void init_mesh_pipeline();
	void init_default_data();
	
	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP];}
	
	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1700 , 900 };
	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffect{0};

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
	
	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
	
	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void destroy_buffer(const AllocatedBuffer& buffer);
	GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
private:
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();
	void init_descriptors();
	void init_pipelines();
	void init_background_pipelines();
	void init_imgui();
	
	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();
	
	void draw_geometry(VkCommandBuffer cmd);

	void draw_background(VkCommandBuffer cmd);
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
};


