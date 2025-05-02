#pragma once 
#include <vk_types.h>
#include <vk_pipelines.h>
#include <fstream>
#include <vk_initializers.h>

namespace vkutil {

    bool load_shader_module(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
};