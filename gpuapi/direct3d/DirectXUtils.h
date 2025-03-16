/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_GPUAPI_DIRECTX_UTILS_H
#define TOS_GPUAPI_DIRECTX_UTILS_H

#include "../../stdlib/Types.h"
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcommon.h>
#include "../../../GameEngine/log/Log.h"
#include "../../../GameEngine/memory/RingMemory.h"
#include "../../../GameEngine/object/Texture.h"
#include "../../../GameEngine/image/Image.cpp"
#include "../../compiler/CompilerUtils.h"
// #include "../../../EngineDependencies/directx/d3d12.h"
// #include "../../../EngineDependencies/directx/d3dx12.h"
#include "FramesInFlightContainer.h"

// Replacement for the windows macro IID_PPVOID
#define IID_PPVOID(pointer) __uuidof(**(pointer)), (void **) (pointer)

bool is_directx_supported(D3D_FEATURE_LEVEL version)
{
    IDXGIFactory6* factory = NULL;
    if (FAILED(CreateDXGIFactory1(IID_PPVOID(&factory)))) {
        return false;
    }

    bool is_dx12_supported = false;

    IDXGIAdapter1* adapter = NULL;
    for (uint32 i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter); ++i) {
        DXGI_ADAPTER_DESC1 desc;
        if (FAILED(adapter->GetDesc1(&desc))) {
            adapter->Release();
            continue;
        }

        // Skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            adapter->Release();
            continue;
        }

        // Check for DirectX 12 support
        if (SUCCEEDED(D3D12CreateDevice(adapter, version, _uuidof(ID3D12Device), NULL))) {
            is_dx12_supported = true;
            adapter->Release();
            break;
        }

        adapter->Release();
    }

    factory->Release();

    return is_dx12_supported;
}

int32 max_directx_version()
{
    if (is_directx_supported(D3D_FEATURE_LEVEL_12_2)) {
        return 122;
    } else if (is_directx_supported(D3D_FEATURE_LEVEL_12_1)) {
        return 121;
    } else if (is_directx_supported(D3D_FEATURE_LEVEL_12_0)) {
        return 120;
    } else if (is_directx_supported(D3D_FEATURE_LEVEL_11_1)) {
        return 111;
    } else if (is_directx_supported(D3D_FEATURE_LEVEL_11_0)) {
        return 110;
    } else if (is_directx_supported(D3D_FEATURE_LEVEL_10_1)) {
        return 101;
    } else if (is_directx_supported(D3D_FEATURE_LEVEL_10_0)) {
        return 100;
    } else if (is_directx_supported(D3D_FEATURE_LEVEL_9_3)) {
        return 93;
    } else if (is_directx_supported(D3D_FEATURE_LEVEL_9_2)) {
        return 92;
    } else if (is_directx_supported(D3D_FEATURE_LEVEL_9_1)) {
        return 91;
    } else if (is_directx_supported(D3D_FEATURE_LEVEL_1_0_CORE)) {
        return 90;
    }

    return 0;
}

inline
void change_viewport(
    int32 width, int32 height,
    ID3D12GraphicsCommandList* command_buffer, D3D12_VIEWPORT* viewport, D3D12_RECT* scissor_rect
)
{
    viewport->Width = (f32) width;
    viewport->Height = (f32) height;

    scissor_rect->right = width;
    scissor_rect->bottom = height;

    command_buffer->RSSetViewports(1, viewport);
    command_buffer->RSSetScissorRects(1, scissor_rect);
}

// Returns frame index
int32 wait_for_previous_frame(
    FramesInFlightContainer* frames_in_flight,
    ID3D12CommandQueue* graphics_queue, IDXGISwapChain3* swapchain
)
{
    // @todo WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    uint64 fence_value_temp = frames_in_flight->fence_value;

    HRESULT hr;

    // Signal and increment the fence value.
    if(FAILED(hr = graphics_queue->Signal(frames_in_flight->fence, fence_value_temp))) {
        LOG_1("DirectX12 Signal: %d", {{LOG_DATA_INT32, &hr}});
        ASSERT_SIMPLE(false);
    }

    ++frames_in_flight->fence_value;

    // Wait until the previous frame is finished.
    if (frames_in_flight->fence->GetCompletedValue() < fence_value_temp) {
        if (FAILED(hr = frames_in_flight->fence->SetEventOnCompletion(fence_value_temp, frames_in_flight->fence_event))) {
            LOG_1("DirectX12 SetEventOnCompletion: %d", {{LOG_DATA_INT32, &hr}});
            ASSERT_SIMPLE(false);
        }

        WaitForSingleObject(frames_in_flight->fence_event, INFINITE);
    }

    return swapchain->GetCurrentBackBufferIndex();
}

static
void directx_debug_callback(
    D3D12_MESSAGE_CATEGORY category,
    D3D12_MESSAGE_SEVERITY severity,
    D3D12_MESSAGE_ID id,
    LPCSTR description,
    void* context
) {
    // @todo handle severity
    (void) category;
    (void) severity;
    (void) id;
    (void*) context;
    /*
    if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        || (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    ) {

    }
    */

    LOG_1(description);
    ASSERT_SIMPLE(false);
}

void gpuapi_debug_messenger_setup(ID3D12Device* device)
{
    ID3D12InfoQueue1* info_queue;
    if (FAILED(device->QueryInterface(IID_PPVOID(&info_queue)))) {
        return;
    }

    // Register the custom debug callback
    info_queue->RegisterMessageCallback(
        directx_debug_callback,
        D3D12_MESSAGE_CALLBACK_FLAG_NONE,
        NULL, // Context (can be used to pass additional data)
        NULL // Callback cookie
    );

    info_queue->SetMessageCountLimit(0);
    info_queue->Release();
}

inline
void gpuapi_pick_physical_device(IDXGIFactory6* instance, IDXGIAdapter1** physical_device, bool requestHighPerformanceAdapter = true)
{
    IDXGIAdapter1* adapter = NULL;
    IDXGIFactory6* factory6 = NULL;

    if (SUCCEEDED(instance->QueryInterface(IID_PPVOID(&factory6)))) {
        for (uint32 adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPVOID(&adapter))
            );
            ++adapterIndex
        ) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), NULL))) {
                break;
            }
        }
    }

    if(!adapter) {
        for (uint32 adapterIndex = 0; SUCCEEDED(instance->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), NULL))) {
                break;
            }
        }
    }

    *physical_device = adapter;
    if (factory6) {
        factory6->Release();
    }
}

inline
void gpuapi_create_logical_device(IDXGIAdapter1* physical_device, ID3D12Device** device)
{
    HRESULT hr;
    if (FAILED(hr = D3D12CreateDevice(physical_device, D3D_FEATURE_LEVEL_11_0, IID_PPVOID(device)))) {
        LOG_1("DirectX12 D3D12CreateDevice: %d", {{LOG_DATA_INT32, &hr}});
        ASSERT_SIMPLE(false);
    }
}

inline
void gpuapi_command_buffer_create(
    ID3D12Device* device,
    ID3D12CommandAllocator* command_pool,
    ID3D12PipelineState* pipeline,
    ID3D12GraphicsCommandList** command_buffer
)
{
    HRESULT hr;
    if (FAILED(hr = device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        command_pool, pipeline,
        IID_PPVOID(command_buffer)))
    ) {
        LOG_1("DirectX12 CreateCommandList: %d", {{LOG_DATA_INT32, &hr}});
        ASSERT_SIMPLE(false);
    };
}

static
DXGI_FORMAT gpuapi_texture_format(byte settings)
{
    if ((settings & IMAGE_SETTING_CHANNEL_4_SIZE)) {
        switch (settings & IMAGE_SETTING_CHANNEL_COUNT) {
            case 1:
                return DXGI_FORMAT_R32_FLOAT;
            case 2:
                return DXGI_FORMAT_R32G32_FLOAT;
            case 3:
                return DXGI_FORMAT_R32G32B32_FLOAT;
            case 4:
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            default:
                UNREACHABLE();
        }
    } else {
        switch (settings & IMAGE_SETTING_CHANNEL_COUNT) {
            case 1:
                return DXGI_FORMAT_R8_UNORM;
            case 2:
                return DXGI_FORMAT_R8G8_UNORM;
            case 3:
                // RGB is not supported (probably due to the alignment
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            case 4:
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            default:
                UNREACHABLE();
        }
    }
}

// @performance Sometimes we want to upload multiple textures in one go (more performant). Allow that or don't use this function in that case.
D3D12_CPU_DESCRIPTOR_HANDLE load_texture_to_gpu(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* command_buffer,
    ID3D12Resource** texture_upload_heap,
    ID3D12Resource** texture_resource,
    int32 descriptor_offset,
    ID3D12DescriptorHeap* srv_heap,
    const Texture* texture,
    RingMemory* ring
) {
    DXGI_FORMAT texture_format = gpuapi_texture_format(texture->image.image_settings);

    D3D12_RESOURCE_DESC texture_info = {};
    texture_info.MipLevels = 1;
    texture_info.Format = texture_format;
    texture_info.Width = texture->image.width;
    texture_info.Height = texture->image.height;
    texture_info.Flags = D3D12_RESOURCE_FLAG_NONE;
    texture_info.DepthOrArraySize = 1;
    texture_info.SampleDesc.Count = 1;
    texture_info.SampleDesc.Quality = 0;
    texture_info.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    D3D12_HEAP_PROPERTIES texture_heap_property = {
        .Type = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 1,
        .VisibleNodeMask = 1
    };

    HRESULT hr;
    if (FAILED(hr = device->CreateCommittedResource(
        &texture_heap_property,
        D3D12_HEAP_FLAG_NONE,
        &texture_info,
        D3D12_RESOURCE_STATE_COPY_DEST,
        NULL,
        IID_PPVOID(texture_resource)))
    ) {
        LOG_1("DirectX12 CreateCommittedResource: %d", {{LOG_DATA_INT32, &hr}});
        ASSERT_SIMPLE(false);

        return {0};
    }

    const D3D12_RESOURCE_DESC destination_info = (*texture_resource)->GetDesc();
    uint64 upload_buffer_size = 0;
    ID3D12Device* device_temp = NULL;
    (*texture_resource)->GetDevice(IID_PPVOID(&device_temp));
    device_temp->GetCopyableFootprints(&destination_info, 0, 1, 0, NULL, NULL, NULL, &upload_buffer_size);

    D3D12_RESOURCE_DESC texture_upload_buffer = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = upload_buffer_size,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0,
        },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE
    };

    D3D12_HEAP_PROPERTIES texture_upload_heap_property = {
        .Type = D3D12_HEAP_TYPE_UPLOAD,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 1,
        .VisibleNodeMask = 1
    };

    texture_heap_property.Type = D3D12_HEAP_TYPE_UPLOAD;
    if (FAILED(hr = device->CreateCommittedResource(
        &texture_heap_property,
        D3D12_HEAP_FLAG_NONE,
        &texture_upload_buffer,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL,
        IID_PPVOID((*texture_upload_heap))))
    ) {
        if (device_temp) {
            device_temp->Release();
        }

        LOG_1("DirectX12 CreateCommittedResource: %d", {{LOG_DATA_INT32, &hr}});
        ASSERT_SIMPLE(false);

        return {0};
    }

    int32 pixel_size = image_pixel_size_from_type(texture->image.image_settings);
    D3D12_SUBRESOURCE_DATA texture_data[] = {
        {
            .pData = texture->image.pixels,
            .RowPitch = texture->image.width * pixel_size,
            .SlicePitch = (texture->image.width * pixel_size) * texture->image.height,
        }
    };

    uint32 number_of_resources = ARRAY_COUNT(texture_data);
    uint32 first_subresource = 0;
    uint64 intermediate_offset = 0;
    uint64 required_size = 0;
    uint64 mem_size = (uint64) (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(uint32) + sizeof(uint64)) * number_of_resources;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts = (D3D12_PLACED_SUBRESOURCE_FOOTPRINT *) ring_get_memory(ring, mem_size, 64);
    uint64* row_size = (uint64 *) (layouts + number_of_resources);
    uint32* row_num = (uint32 *) (row_size + number_of_resources);

    device_temp->GetCopyableFootprints(&destination_info, first_subresource, number_of_resources, intermediate_offset, layouts, row_num, row_size, &required_size);
    device_temp->Release();

    const D3D12_RESOURCE_DESC intermediate_info = (*texture_upload_heap)->GetDesc();
    if (intermediate_info.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER
        || intermediate_info.Width < required_size + layouts[0].Offset
        || required_size > ((size_t) -1)
        || (destination_info.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
                (first_subresource != 0 || number_of_resources != 1)
        )
    ) {
        LOG_1("DirectX12 texture resource setup");
        ASSERT_SIMPLE(false);

        return {0};
    }

    byte* data;
    // @question is 0 correct here? Even for multiple function calls?
    if (FAILED(hr = texture_upload_heap->Map(0, NULL, (void **) &data))) {
        LOG_1("DirectX12 Map: %d", {{LOG_DATA_INT32, &hr}});
        ASSERT_SIMPLE(false);

        return {0};
    }

    for (uint32 i = 0; i < number_of_resources; ++i) {
        ASSERT_SIMPLE(row_size[i] <= ((size_t) -1));

        D3D12_MEMCPY_DEST DestData = { data + layouts[i].Offset, layouts[i].Footprint.RowPitch, ((size_t) layouts[i].Footprint.RowPitch) * ((size_t) row_num[i]) };
        for (uint32 z = 0; z < layouts[i].Footprint.Depth; ++z) {
            byte* dest_slize = ((byte *) DestData.pData) + DestData.SlicePitch * z;
            byte* src_slice = ((byte *) texture_data[i].pData) + texture_data[i].SlicePitch * ((intptr_t) z);
            for (uint32 y = 0; y < row_num[i]; ++y) {
                memcpy(
                    dest_slize + DestData.RowPitch * y,
                    src_slice + texture_data[i].RowPitch * ((intptr_t) y),
                    (size_t) row_size[i]
                );
            }
        }
    }
    (*texture_upload_heap)->Unmap(0, NULL);

    if (destination_info.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        command_buffer->CopyBufferRegion(
            *texture_resource, 0, *texture_upload_heap, layouts[0].Offset, layouts[0].Footprint.Width
        );
    } else {
        for (uint32 i = 0; i < number_of_resources; ++i) {
            D3D12_TEXTURE_COPY_LOCATION dst = {
                .pResource = *texture_resource,
                .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                .SubresourceIndex = i + first_subresource,
            };

            D3D12_TEXTURE_COPY_LOCATION src = {
                .pResource = *texture_upload_heap,
                .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                .PlacedFootprint = layouts[i],
            };

            command_buffer->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);
        }
    }

    D3D12_RESOURCE_BARRIER barrier = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource = *texture_resource,
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
            .StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        }
    };
    command_buffer->ResourceBarrier(1, &barrier);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_info = {};
    srv_info.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_info.Format = texture_info.Format;
    srv_info.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_info.Texture2D.MipLevels = 1;

    D3D12_CPU_DESCRIPTOR_HANDLE srv_handle = srv_heap->GetCPUDescriptorHandleForHeapStart();
    device->CreateShaderResourceView(*texture_resource, &srv_info, srv_handle);

    srv_handle.ptr += descriptor_offset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    return srv_handle;
}

void gpuapi_vertex_buffer_create(
    ID3D12Device* device,
    D3D12_VERTEX_BUFFER_VIEW* vertex_buffer_view,
    ID3D12Resource** vertex_buffer,
    const void* __restrict vertices, uint32 vertex_size, uint32 vertex_count
)
{
    D3D12_RESOURCE_DESC resource_info = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = vertex_size * vertex_count,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE
    };

    // Note: using upload heaps to transfer static data like vert buffers is not
    // recommended. Every time the GPU needs it, the upload heap will be marshalled
    // over. Please read up on Default Heap usage. An upload heap is used here for
    // code simplicity and because there are very few verts to actually transfer.
    D3D12_HEAP_PROPERTIES heap_property = {
        .Type = D3D12_HEAP_TYPE_UPLOAD,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 1,
        .VisibleNodeMask = 1
    };

    HRESULT hr;
    if (FAILED(hr = device->CreateCommittedResource(
        &heap_property,
        D3D12_HEAP_FLAG_NONE,
        &resource_info,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL,
        IID_PPVOID(vertex_buffer)))
    ) {
        LOG_1("DirectX12 CreateCommittedResource: %d", {{LOG_DATA_INT32, &hr}});
        ASSERT_SIMPLE(false);

        return;
    }

    // Copy the triangle data to the vertex buffer
    uint8* vertex_data_begin;
    // We do not intend to read from this resource on the CPU
    D3D12_RANGE read_range = {};
    if (FAILED(hr = (*vertex_buffer)->Map(0, &read_range, (void **) &vertex_data_begin))) {
        LOG_1("DirectX12 Map: %d", {{LOG_DATA_INT32, &hr}});
        ASSERT_SIMPLE(false);
    }

    memcpy(vertex_data_begin, vertices, vertex_size * vertex_count);
    (*vertex_buffer)->Unmap(0, NULL);

    // Initialize the vertex buffer view
    vertex_buffer_view->BufferLocation = (*vertex_buffer)->GetGPUVirtualAddress();
    vertex_buffer_view->StrideInBytes = vertex_size;
    vertex_buffer_view->SizeInBytes = vertex_size * vertex_count;
}

// @question vertex_count is a count where offset is bytes, this seems inconsistent
void gpuapi_vertex_buffer_update(
    ID3D12Resource* vertex_buffer,
    const void* __restrict vertices,
    uint32 vertex_size,
    uint32 vertex_count,
    uint32 offset = 0
)
{
    uint64 size = vertex_count * vertex_size - offset;

    uint8* vertex_data_begin;
    D3D12_RANGE read_range = {};
    D3D12_RANGE write_range = { offset, offset + size };

    HRESULT hr;
    if (FAILED(hr = vertex_buffer->Map(0, &read_range, (void**) &vertex_data_begin))) {
        LOG_1("DirectX12 Map: %d", {{LOG_DATA_INT32, &hr}});
        ASSERT_SIMPLE(false);
        return;
    }

    // @bug Isn't this wrong? isn't size - offset?
    memcpy(vertex_data_begin + offset, ((byte *) vertices) + offset, size);

    vertex_buffer->Unmap(0, &write_range);
}

// In directx this is actually called a constant buffer
void gpuapi_uniform_buffers_create(
    ID3D12Device* device,
    ID3D12Resource** uniform_buffer,
    const void* __restrict data, uint32 buffer_size
)
{
    D3D12_RESOURCE_DESC resource_info = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = buffer_size,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE
    };

    // Note: using upload heaps to transfer static data like vert buffers is not
    // recommended. Every time the GPU needs it, the upload heap will be marshalled
    // over. Please read up on Default Heap usage. An upload heap is used here for
    // code simplicity and because there are very few verts to actually transfer.
    D3D12_HEAP_PROPERTIES heap_property = {
        .Type = D3D12_HEAP_TYPE_UPLOAD,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 1,
        .VisibleNodeMask = 1
    };

    device->CreateCommittedResource(
        &heap_property,
        D3D12_HEAP_FLAG_NONE,
        &resource_info,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL,
        IID_PPV_ARGS(uniform_buffer));

    D3D12_RANGE read_range = {};

    uint8* data_begin;
    (*uniform_buffer)->Map(0, &read_range, (void **) &data_begin);
    memcpy(data_begin, &data, buffer_size);
    (*uniform_buffer)->Unmap(0, NULL);
}

void gpuapi_uniform_buffer_update(
    ID3D12Resource* uniform_buffer,
    const void* __restrict data,
    uint32 buffer_size
)
{
    D3D12_RANGE read_range = {};
    uint8* data_begin = NULL;
    uniform_buffer->Map(0, &read_range, (void **) &data_begin);

    memcpy(data_begin, data, buffer_size);

    uniform_buffer->Unmap(0, NULL);
}

#endif