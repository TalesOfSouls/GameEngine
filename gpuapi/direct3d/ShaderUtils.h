/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_DIRECTX_SHADER_UTILS_H
#define COMS_GPUAPI_DIRECTX_SHADER_UTILS_H

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include "../../stdlib/Types.h"
#include "../../memory/RingMemory.h"
#include "../../log/Log.h"
#include "../../log/Stats.h"
#include "../../log/PerformanceProfiler.h"
#include "../../object/Vertex.h"
#include "../../utils/StringUtils.h"
#include "../../log/Log.h"
#include "../ShaderType.h"
#include "../GpuAttributeType.h"

#pragma comment(lib, "d3dcompiler.lib")

const char* shader_type_index(ShaderType type)
{
    switch (type) {
        case SHADER_TYPE_VERTEX:
            return "vs_5_0";
        case SHADER_TYPE_FRAGMENT:
            return "ps_5_0";
        default:
            UNREACHABLE();
    }
}

ID3DBlob* gpuapi_shader_make(const char* type, const char* source, int32 source_size)
{
    LOG_1("Create shader");
    #if DEBUG || INTERNAL
        uint32 compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #else
        uint32 compileFlags = 0;
    #endif

    ID3DBlob* blob;
    ID3DBlob* errMsgs;
    HRESULT hr;

    if (FAILED(hr = D3DCompile2(source, source_size, NULL, NULL, NULL, "main", type, compileFlags, 0, 0, NULL, 0, &blob, &errMsgs))) {
        LOG_1("DirectX12 D3DCompile2: %d, %s", {{LOG_DATA_INT32, &hr}, {LOG_DATA_CHAR_STR, errMsgs->GetBufferPointer()}});
        ASSERT_SIMPLE(false);
    }

    if (errMsgs) {
        errMsgs->Release();
    }

    LOG_1("Created shader");

    return blob;
}


ID3D12PipelineState* gpuapi_pipeline_make(
    ID3D12Device* device,
    ID3D12PipelineState** pipeline,
    ID3D12RootSignature* pipeline_layout,
    D3D12_INPUT_ELEMENT_DESC* descriptor_set_layouts, uint32 layout_count,
    ID3DBlob* vertex_shader,
    ID3DBlob* fragment_shader,
    ID3DBlob*
) {
    PROFILE(PROFILE_PIPELINE_MAKE, NULL, false, true);
    LOG_1("Create pipeline");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_info = {};
    pipeline_state_info.InputLayout = { descriptor_set_layouts, layout_count };
    pipeline_state_info.pRootSignature = pipeline_layout;
    pipeline_state_info.VS = {
        .pShaderBytecode = vertex_shader->GetBufferPointer(),
        .BytecodeLength = vertex_shader->GetBufferSize()
    };
    pipeline_state_info.PS = {
        .pShaderBytecode = fragment_shader->GetBufferPointer(),
        .BytecodeLength = fragment_shader->GetBufferSize()
    };
    pipeline_state_info.RasterizerState = {
        .FillMode = D3D12_FILL_MODE_SOLID,
        .CullMode = D3D12_CULL_MODE_BACK,
        .FrontCounterClockwise = false,
        .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable = true,
        .MultisampleEnable = false,
        .AntialiasedLineEnable = false,
        .ForcedSampleCount = 0,
        .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
    };

    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
        false,false,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    pipeline_state_info.BlendState = {
        .AlphaToCoverageEnable = false,
        .IndependentBlendEnable = false,
        .RenderTarget = {
            defaultRenderTargetBlendDesc, defaultRenderTargetBlendDesc, defaultRenderTargetBlendDesc, defaultRenderTargetBlendDesc,
            defaultRenderTargetBlendDesc, defaultRenderTargetBlendDesc, defaultRenderTargetBlendDesc, defaultRenderTargetBlendDesc
        }
    };
    pipeline_state_info.DepthStencilState.DepthEnable = false;
    pipeline_state_info.DepthStencilState.StencilEnable = false;
    pipeline_state_info.SampleMask = UINT_MAX;
    pipeline_state_info.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline_state_info.NumRenderTargets = 1;
    pipeline_state_info.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipeline_state_info.SampleDesc.Count = 1;

    HRESULT hr;
    if (FAILED(hr = device->CreateGraphicsPipelineState(&pipeline_state_info, IID_PPV_ARGS(pipeline)))) {
        LOG_1("DirectX12 CreateGraphicsPipelineState: %d", {{LOG_DATA_INT32, &hr}});
        ASSERT_SIMPLE(false);
    }

    // @question When do I ->Release() vertex_shader and fragment_shader?
    LOG_1("Created pipeline");

    return *pipeline;
}

FORCE_INLINE
void gpuapi_pipeline_use(ID3D12GraphicsCommandList* command_buffer, ID3D12PipelineState* pipelineState)
{
    command_buffer->SetPipelineState(pipelineState);
}

// In DirectX Attribute info and descriptor set layout are combined into one
constexpr
void gpuapi_attribute_info_create(GpuAttributeType type, D3D12_INPUT_ELEMENT_DESC* attr)
{
    switch (type) {
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D: {
            attr[0] = {
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex3D, position),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };

            attr[1] = {
                .SemanticIndex = 1,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex3D, normal),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };

            attr[2] = {
                .SemanticIndex = 2,
                .Format = DXGI_FORMAT_R32G32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex3D, tex_coord),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };

            attr[3] = {
                .SemanticIndex = 3,
                .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex3D, color),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_NORMAL: {
            attr[0] = {
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex3DNormal, position),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };

            attr[1] = {
                .SemanticIndex = 1,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex3DNormal, normal),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_COLOR: {
            attr[0] = {
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex3DColor, position),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };

            attr[1] = {
                .SemanticIndex = 1,
                .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex3DColor, color),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_TEXTURE_COLOR: {
            attr[0] = {
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex3DTextureColor, position),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };

            attr[1] = {
                .SemanticIndex = 1,
                .Format = DXGI_FORMAT_R32G32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex3DTextureColor, texture_color),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_SAMPLER_TEXTURE_COLOR: {
            attr[0] = {
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32B32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex3DSamplerTextureColor, position),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };

            attr[1] = {
                .SemanticIndex = 1,
                .Format = DXGI_FORMAT_R32_SINT,
                .AlignedByteOffset = offsetof(Vertex3DSamplerTextureColor, sampler),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };

            attr[2] = {
                .SemanticIndex = 2,
                .Format = DXGI_FORMAT_R32G32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex3DSamplerTextureColor, texture_color),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_2D_TEXTURE: {
            attr[0] = {
                .SemanticIndex = 0,
                .Format = DXGI_FORMAT_R32G32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex2DTexture, position),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };

            attr[1] = {
                .SemanticIndex = 1,
                .Format = DXGI_FORMAT_R32G32_FLOAT,
                .AlignedByteOffset = offsetof(Vertex2DTexture, tex_coord),
                .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                .InstanceDataStepRate = 0,
            };
        } return;
        default:
            UNREACHABLE();
    };
}

int32 directx_program_optimize(const char* input, char* output)
{
    const char* read_ptr = input;
    char* write_ptr = output;
    bool in_string = false;

    while (*read_ptr) {
        str_skip_empty(&read_ptr);

        if (write_ptr != output
            && *(write_ptr - 1) != '\n' && *(write_ptr - 1) != ';' && *(write_ptr - 1) != '{'
            && *(write_ptr - 1) != '('
            && *(write_ptr - 1) != ','
        ) {
            *write_ptr++ = '\n';
        }

        // Handle single-line comments (//)
        if (*read_ptr == '/' && *(read_ptr + 1) == '/' && !in_string) {
            str_move_to(&read_ptr, '\n');

            continue;
        }

        // Handle multi-line comments (/* */)
        if (*read_ptr == '/' && *(read_ptr + 1) == '*' && !in_string) {
            // Go to end of comment
            while (*read_ptr && (*read_ptr != '*' || *(read_ptr + 1) != '/')) {
                ++read_ptr;
            }

            if (*read_ptr == '*' && *(read_ptr + 1) == '/') {
                read_ptr += 2;
            }

            continue;
        }

        // Handle strings to avoid removing content within them
        if (*read_ptr == '"') {
            in_string = !in_string;
        }

        // Copy valid characters to write_ptr
        while (*read_ptr && !is_eol(read_ptr) && *read_ptr != '"'
            && !(*read_ptr == '/' && (*(read_ptr + 1) == '/' || *(read_ptr + 1) == '*'))
        ) {
            if (!in_string
                && (*read_ptr == '*' || *read_ptr == '/' || *read_ptr == '=' || *read_ptr == '+' || *read_ptr == '-' || *read_ptr == '%'
                    || *read_ptr == '(' || *read_ptr == ')'
                    || *read_ptr == '{' || *read_ptr == '}'
                    || *read_ptr == ',' || *read_ptr == '?' || *read_ptr == ':' || *read_ptr == ';'
                    || *read_ptr == '&' || *read_ptr == '|'
                    || *read_ptr == '>' || *read_ptr == '<'
                )
            ) {
                if (is_whitespace(*(write_ptr - 1)) || *(write_ptr - 1) == '\n') {
                    --write_ptr;
                }

                *write_ptr++ = *read_ptr++;

                if (*read_ptr && is_whitespace(*read_ptr)) {
                    ++read_ptr;
                }
            } else {
                *write_ptr++ = *read_ptr++;
            }
        }
    }

    *write_ptr = '\0';

    // -1 to remove \0 from length, same as strlen
    return (int32) (write_ptr - output);
}

#endif