/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_VULKAN_GPU_INFO_C
#define COMS_GPUAPI_VULKAN_GPU_INFO_C

#include "../../stdlib/Types.h"
#include "../../utils/StringUtils.h"
#include "../GPUInfo.h"
#include <d3d12.h>
#include <dxgi1_6.h>

uint64 gpu_info_features() {
    uint64 features = 0;

    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options))))
    {
        // Bindless textures ↔ Resource Binding Tier 2 or above
        if (options.ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_2) {
            features |= GPU_FEATURE_BINDLESS_TEXTURE;
        }

        // Sparse textures ↔ "Tiled Resources"
        if (options.TiledResourcesTier != D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED) {
            features |= GPU_FEATURE_SPARSE_TEXTURE;
        }
    }

    // In DX12, upload heap buffers can always be persistently mapped.
    features |= GPU_FEATURE_PERSISTENT_BUFFERS;

    // DX12 always allows UAV/SRV structured buffers and byte address buffers.
    // This is the equivalent of SSBOs → always supported.
    features |= GPU_FEATURE_SHADER_STORAGE_BUFFER_OBJECTS;

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))))
    {
        if (options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            features |= GPU_FEATURE_RAY_TRACING_DXR;
            features |= GPU_FEATURE_RAY_TRACING_ACCEL_BLAS;
            features |= GPU_FEATURE_RAY_TRACING_ACCEL_TLAS;
        }
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
    if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6))))
    {
        if (options6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED)
        {
            features |= GPU_FEATURE_VARIABLE_RATE_SHADING;
        }
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
    if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7))))
    {
        if (options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
        {
            features |= GPU_FEATURE_MASH_SHADERS;
        }
    }

    // All modern DX12 GPUs support BC (DXTC / S3TC) compression.
    features |= GPU_FEATURE_COMPRESSION_BC;

    // ASTC and ETC are generally NOT supported in DX12 (PC GPUs).

    // All DirectX 11+ hardware supports anisotropic filtering (guaranteed by the spec),
    features |= GPU_FEATURE_ANISOTROPIC_FILTERING;

    // Texture gather: mandatory in Shader Model 4.0+
    features |= GPU_FEATURE_TEXTURE_GATHER;

    // Texture arrays and cube map arrays are supported in DX10+ → always on.
    features |= GPU_FEATURE_TEXTURE_ARRAYS;

    // Indirect draw: DX12 core supports ExecuteIndirect
    features |= GPU_FEATURE_INDIRECT_DRAW;

    // Occlusion queries: core DX12 feature, just like in DX11
    features |= GPU_FEATURE_OCCLUSION_RENDERING;

    // Transform feedback (stream output): available in DX12 via Stream Output stage
    features |= GPU_FEATURE_TRANSFORM_FEEDBACK;

    // @todo
    // Cross-GPU shared resources: if device is part of a linked adapter, can share.
    // For simplicity: not checked here. You'd check IDXGIFactory4::EnumAdapterByLuid or device groups.
    // features |= GPU_FEATURE_CROSS_GPU_SHARED_RESOURCES;  <-- only if multi-adapter configured

    // Async compute: always supported in DX12. You can create separate compute queues.
    features |= GPU_FEATURE_ASYNC_COMPUTE;

    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_5 };
    if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
    {
        // FP64 support is guaranteed at feature level 11_0 and up (double precision).
        features |= GPU_FEATURE_FP64;

        // FP16 support depends on optional shader model / extensions (Shader Model 6.2+ typically).
        // Most modern GPUs support min16float. There's no explicit feature flag check in DX12,
        // but shader model ≥ 6.2 usually implies it.
        if (shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_2)
            features |= GPU_FEATURE_FP16;

        // INT16 and INT64 are also tied to shader model capabilities.
        if (shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_2)
            features |= GPU_FEATURE_SHADER_INT16;

        if (shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_0)
            features |= GPU_FEATURE_SHADER_INT64;

        // Bitfield operations are part of SM 5.1+ → available on DX12.
        features |= GPU_FEATURE_SHADER_BITFIELD;

        // Variable precision (min16float/int) is also part of SM 6.2+.
        if (shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_2)
            features |= GPU_FEATURE_VARIABLE_PRECISION;
    }

    return features;
}

void gpuapi_info_get(GpuInfo* info, ID3D12Device* device) {
    IDXGIDevice* dxgiDevice = nullptr;
    if (SUCCEEDED(device->QueryInterface(__uuidof(IDXGIDevice), (void**) &dxgiDevice))) {
        IDXGIAdapter* adapter = nullptr;
        if (SUCCEEDED(dxgiDevice->GetAdapter(&adapter))) {
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(adapter->GetDesc(&desc))) {
                wchar_to_char(info->name, desc.Description, sizeof(info->name) - 1);

                //sprintf_fast(info->vendor, sizeof(info->vendor) - 1, "%04X", desc.VendorId);

                info->vram = (uint32) (desc.DedicatedVideoMemory / (1024 * 1024));
            }

            adapter->Release();
        }

        dxgiDevice->Release();
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)))) {
        sprintf_fast(info->api_version, sizeof(info->api_version) - 1, "D3D12");
        sprintf_fast(info->shader_version, sizeof(info->shader_version) - 1, "SM %d.%d", 6, 0);
    }

    D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT addrSupport = {};
    if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &addrSupport, sizeof(addrSupport)))) {
        info->alignment = 256;
    }

    info->features = gpu_info_features();
}

#endif