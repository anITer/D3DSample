#include "d3drender.h"

#include <d3dcompiler.h>
#include <directxmath.h>
#include <stdexcept>

struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
    ComPtr<IDXGIAdapter1> adapter;
    *ppAdapter = nullptr;

    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            // If you want a software adapter, pass in "/warp" on the command line.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

    *ppAdapter = adapter.Detach();
}

void D3D12Render::LoadPipeline(HWND hWnd)
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Step1：图形调试器支持
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif
    // Step2：创建设备
    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(factory.Get(), &hardwareAdapter);

    D3D12CreateDevice(
        hardwareAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&pd3dDevice)
    );

    // Step3：创建命令队列（GPU和CPU通信，使用一堆命令队列依次执行，是否可以理解为，CPU和GPU通信方式修改成了异步方式了？）
    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    pd3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pd3dCommandQueue));

    // Step4：创建交换链
    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = frameCnt;      // 至少有两个
    swapChainDesc.Width = 640;
    swapChainDesc.Height = 480;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        pd3dCommandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));
    swapChain.As(&pd3dSwapChain);

    // Step5：创建渲染器目标视图(RTV) 描述符堆，类似一个资源池，可以用来分配给渲染目标等。
    // Describe and create a render target view (RTV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = frameCnt;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    pd3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&prtvDescHeap));
    rtvDescriptorSize = pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Step6：创建帧资源（每个帧的渲染器目标视图），每次创建，都需要找描述符堆申请资源
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(prtvDescHeap->GetCPUDescriptorHandleForHeapStart());

    for (UINT n = 0; n < frameCnt; n++)
    {
        ThrowIfFailed(pd3dSwapChain->GetBuffer(n, IID_PPV_ARGS(&pRenderTarget[n])));
        pd3dDevice->CreateRenderTargetView(pRenderTarget[n].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, rtvDescriptorSize);
    }

    // Step7：创建命令分配器，管理命令列表
    pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pd3dCommandAllocator));
}

void D3D12Render::LoadAssets()
{
    // Step8：创建根签名，类似签名证书根证书一样，资源一旦“打”上这个签名（结构体中指向根签名对象），就表示要进入渲染管道进行渲染使用
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    pd3dDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSign));

    // Step9：编译着色器
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
    ThrowIfFailed(D3DCompileFromFile(L"D:\\D3DSample\\D3D12Test\\shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"D:\\D3DSample\\D3D12Test\\shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

    // Step10：定义顶点输入布局
    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    pViewport.reset(new CD3DX12_VIEWPORT(0.0f, 0.0f, 640.0f, 480.0f));
    pScissorRect.reset(new CD3DX12_RECT(0.0f, 0.0f, 640.0f, 480.0f));

    // Step11：创建图形管道状态对象，图形管道状态对象包括顶点着色器、像素着色器、顶点输入布局、顶点拓扑结构、渲染目标格式等
    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = rootSign.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    pd3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));

    // Step12：创建命令列表
    pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pd3dCommandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&commandList));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    commandList->Close();

    // Step13：创建并加载顶点缓冲区
    Vertex triangleVertices[] =
    {
        { { 0.0f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
    };

    const UINT vertexBufferSize = sizeof(triangleVertices);

    // Note: using upload heaps to transfer static data like vert buffers is not 
    // recommended. Every time the GPU needs it, the upload heap will be marshalled 
    // over. Please read up on Default Heap usage. An upload heap is used here for 
    // code simplicity and because there are very few verts to actually transfer.
    pd3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pVertexBuffer));

    // Copy the triangle data to the vertex buffer.
    UINT8* pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
    pVertexBuffer->Unmap(0, nullptr);

    // Step14：创建顶点缓冲区视图
    // Initialize the vertex buffer view.
    vertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = vertexBufferSize;

    // Step15：创建fence，用于GPU和CPU同步
    pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));
    fenceValue = 1;

    // Create an event handle to use for frame synchronization.
    hfenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (hfenceEvent == nullptr)
    {
        return;
    }

    // Step16：等待GPU完成准备工作
    // Wait for the command list to execute; we are reusing the same command 
    // list in our main loop but for now, we just want to wait for setup to 
    // complete before continuing.

    WaitForPreviousFrame();
}

void D3D12Render::Setup(HWND hWnd)
{
    // 加载管线
    LoadPipeline(hWnd);

    // 记载资产
    LoadAssets();
}

void D3D12Render::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = fenceValue;
    pd3dCommandQueue->Signal(pFence.Get(), fence);
    fenceValue++;

    // Wait until the previous frame is finished.
    if (pFence->GetCompletedValue() < fence)
    {
        pFence->SetEventOnCompletion(fence, hfenceEvent);
        WaitForSingleObject(hfenceEvent, INFINITE);
    }

    frameIndex = pd3dSwapChain->GetCurrentBackBufferIndex();
}

void D3D12Render::PrepareCommandList()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    pd3dCommandAllocator->Reset();

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    commandList->Reset(pd3dCommandAllocator.Get(), pipelineState.Get());

    // Set necessary state.
    commandList->SetGraphicsRootSignature(rootSign.Get());
    commandList->RSSetViewports(1, pViewport.get());
    commandList->RSSetScissorRects(1, pScissorRect.get());

    // Indicate that the back buffer will be used as a render target.
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pRenderTarget[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(prtvDescHeap->GetCPUDescriptorHandleForHeapStart(), 0, rtvDescriptorSize);
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->DrawInstanced(3, 1, 0, 0);

    // Indicate that the back buffer will now be used to present.
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pRenderTarget[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    commandList->Close();
}

void D3D12Render::Draw()
{
    if (!pFence)
    {
        return;
    }
    // Step17：填充命令列表
    PrepareCommandList();

    // Step18：执行命令列表
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    pd3dCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Step19：触发swapchain，显示帧
    pd3dSwapChain->Present(1, 0);

    // Step20：等待GPU完成动作
    WaitForPreviousFrame();
}

void D3D12Render::Release()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForPreviousFrame();

    CloseHandle(hfenceEvent);
    pViewport.reset();
    pScissorRect.reset();
}
