// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <imgui_impl_vulkan.h>
#include <vk_types.h>

#include "camera.h"
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

// 一帧拥有的资源
struct FrameData
{
	VkCommandPool	_command_pool;
	VkCommandBuffer _mainCommandBuffer;

	VkSemaphore		_swapchainSemaphore, _renderSemaphore;
	VkFence			_renderFence;
	
	DescriptorAllocatorGrowable _frameDescriptors;

	DeletionQueue _deletionQueue;
};

struct ComputeEffect {
	const char* name;

	VkPipeline pipeline;
	VkPipelineLayout layout;

	ComputePushConstants data;
};

class VulkanEngine;

struct GLTFMetallic_Roughness {
	MaterialPipeline opaquePipeline;
	MaterialPipeline transparentPipeline;

	VkDescriptorSetLayout materialLayout;

	struct MaterialConstants {
		glm::vec4 colorFactors;
		glm::vec4 metal_rough_factors; // r: metallic, g: roughness, ba: not used.
		//padding, we need it anyway for uniform buffers
		glm::vec4 extra[14];
	};

	struct MaterialResources {
		AllocatedImage  colorImage;
		VkSampler		colorSampler;
		AllocatedImage  metalRoughImage;
		VkSampler		metalRoughSampler;
		VkBuffer		dataBuffer;
		uint32_t		dataBufferOffset;
	};

	DescriptorWriter writer;

	void build_pipelines(VulkanEngine* engine);
	void clear_resources(VkDevice device);

	MaterialInstance write_material(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
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
	bool						_resize_requested { false };
	float						_renderScale {1.f};
	
	// default resources
	AllocatedImage _whiteImage;
	AllocatedImage _blackImage;
	AllocatedImage _greyImage;
	AllocatedImage _errorCheckerboardImage;

	VkSampler      _defaultSamplerLinear;
	VkSampler      _defaultSamplerNearest;
	
	DescriptorAllocatorGrowable	globalDescriptorAllocator;
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
	
	// scene data
	GPUSceneData		  _sceneData;
	VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;
	VkDescriptorSetLayout _singleImageDescriptorLayout;
	
	MaterialInstance       _defaultData;
	GLTFMetallic_Roughness _metalRoughMaterial;
	
	DrawContext mainDrawContext;
	std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;
	Camera mainCamera;
	
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
	AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
	void destroy_image(const AllocatedImage& img);
	
	GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
	void update_scene();
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
	void resize_swapchain();

	void draw_geometry(VkCommandBuffer cmd);

	void draw_background(VkCommandBuffer cmd);
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
};


