#ifndef VULKAN_LUAJIT_H
#define VULKAN_LUAJIT_H

#include "lua.h"
#include <SDL3/SDL.h>        // For SDL_Window in SDL_Vulkan_CreateSurface
#include <SDL3/SDL_vulkan.h> // For SDL_Vulkan_CreateSurface
#include <vulkan/vulkan.h>

typedef struct {
  VkInstance instance;
} VulkanInstance;

typedef struct {
  VkPhysicalDevice physicalDevice;
} VulkanPhysicalDevice;

typedef struct {
  VkDevice device;
} VulkanDevice;

typedef struct {
  VkSurfaceKHR surface;
  VkInstance instance; // Store instance for cleanup
} VulkanSurface;

typedef struct {
  VkQueue queue;
} VulkanQueue;

typedef struct {
  VkSwapchainKHR swapchain;
  VkDevice device; // For cleanup
} VulkanSwapchain;

typedef struct {
  VkImage image;
} VulkanImage;

typedef struct {
  VkImageView imageView;
  VkDevice device;
} VulkanImageView;

typedef struct {
  VkRenderPass renderPass;
  VkDevice device;
} VulkanRenderPass;

typedef struct {
  VkFramebuffer framebuffer;
  VkDevice device;
} VulkanFramebuffer;

typedef struct {
  VkShaderModule shaderModule;
  VkDevice device;
} VulkanShaderModule;

typedef struct {
  VkPipelineLayout pipelineLayout;
  VkDevice device;
} VulkanPipelineLayout;

typedef struct {
  VkPipeline pipeline;
  VkDevice device;
} VulkanPipeline;

typedef struct {
  VkSemaphore semaphore;
  VkDevice device;
} VulkanSemaphore;

typedef struct {
  VkFence fence;
  VkDevice device;
} VulkanFence;

int luaopen_vulkan(lua_State *L);

#endif