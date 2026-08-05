/**
 * Jingga
 *
 * @copyright Jingga
 * @license    License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_ASSET_TYPE_H
#define COMS_GPUAPI_ASSET_TYPE_H

#include "../stdlib/Stdlib.h"

enum GpuAssetType : byte {
    GPU_ASSET_MESH,
    GPU_ASSET_TEXTURE,
    GPU_ASSET_SHADER,
    GPU_ASSET_BUFFER,
};

struct OpenGLMeshData {
    uint32 vao;
    uint32 vbo;
    uint32 ebo;
    uint32 index_count;
    uint32 index_gl_type;
};

struct SoftwareMeshData {
    uint32 vao;
    uint32 vbo;
    uint32 ebo;
    uint32 index_count;
    uint32 index_gl_type;
};

struct VulkanMeshData {
    void* vertex_buffer; // VkBuffer
    void* vertex_memory; // VkDeviceMemory
    void* index_buffer; // VkBuffer
    void* indexMemory; // VkDeviceMemory
    uint32 index_count;
    uint32 index_type;
};

struct D3D12MeshData {
    void* vertex_buffer_resource; // ID3D12Resource*
    void* index_buffer_resource; // ID3D12Resource*
    // D3D12_VERTEX_BUFFER_VIEW / D3D12_INDEX_BUFFER_VIEW are small PODs;
    // store their fields directly once <d3d12.h> is available, e.g.:
    //   D3D12_VERTEX_BUFFER_VIEW vbv;
    //   D3D12_INDEX_BUFFER_VIEW ibv;
    uint64 vertex_buffer_gpu_address;
    uint32 vertex_stride_bytes;
    uint32 vertex_size_bytes;
    uint64 index_buffer_gpu_address;
    uint32 index_size_bytes;
    uint32 index_count;
};

struct MetalMeshData {
    void* vertex_buffer; // id<MTLBuffer>
    void* index_buffer; // id<MTLBuffer>
    uint32 index_count;
    uint32 index_type;
};

struct OpenGLTextureData {
    uint32 texture_id;
    int32 sampler;
};

struct SoftwareTextureData {
    uint32 texture_id;
    uint32 sampler;
};


struct VulkanTextureData {
    void* image; // VkImage
    void* image_view; // VkImageView
    void* memory; // VkDeviceMemory
    void* sampler; // VkSampler
};

struct D3D12TextureData {
    void* resource; // ID3D12Resource*
    uint64 srv_descriptor;
};

struct MetalTextureData {
    void* texture; // id<MTLTexture>
    void* sampler; // id<MTLSamplerState>
};

// similar to GL_MAX_COLOR_ATTACHMENTS
#define MAX_COLOR_ATTACHMENTS 8

struct OpenGLFramebufferData {
    uint32 fbo;
    uint32 rbo; // depth/stencil renderbuffer; 0 if depth is a texture instead, or unused
    uint32 color_textures[MAX_COLOR_ATTACHMENTS]; // GL texture ids, owned by this framebuffer
    uint32 depth_stencil_texture; // 0 if depth/stencil uses `rbo` instead, or unused
    uint32 color_attachment_count;
};

struct VulkanFramebufferData {
    void* framebuffer; // VkFramebuffer
    void* render_pass; // VkRenderPass this framebuffer was created against

    void* color_images[MAX_COLOR_ATTACHMENTS]; // VkImage
    void* color_image_views[MAX_COLOR_ATTACHMENTS]; // VkImageView
    void* color_image_memory[MAX_COLOR_ATTACHMENTS]; // VkDeviceMemory
    uint32 color_attachment_count;

    void* depth_image; // VkImage
    void* depth_image_view; // VkImageView
    void* depth_image_memory; // VkDeviceMemory
};

struct D3D12FramebufferData {
    void* color_resources[MAX_COLOR_ATTACHMENTS]; // ID3D12Resource*
    uint64 rtv_descriptors[MAX_COLOR_ATTACHMENTS]; // D3D12_CPU_DESCRIPTOR_HANDLE.ptr, in the RTV heap
    uint32 color_attachment_count;

    void* depth_resource; // ID3D12Resource*
    uint64 dsv_descriptor; // D3D12_CPU_DESCRIPTOR_HANDLE.ptr, in the DSV heap
};

struct MetalFramebufferData {
    void* color_textures[MAX_COLOR_ATTACHMENTS]; // id<MTLTexture>
    uint32 color_attachment_count;

    void* depth_texture; // id<MTLTexture>
    void* stencil_texture; // id<MTLTexture>, often == depth_texture for combined formats
};

#endif