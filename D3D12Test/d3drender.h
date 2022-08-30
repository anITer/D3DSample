#pragma once
#include "framework.h"
#include "d3dx12.h"
#include <dxgi1_4.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;

class D3D12Render {
public:
	void Setup(HWND hWnd);
	void Draw();
	void Release();

private:
	void LoadPipeline(HWND hWnd);
	void LoadAssets();
	void WaitForPreviousFrame();
	void PrepareCommandList();

	ComPtr<ID3D12Device> pd3dDevice;
	ComPtr<ID3D12CommandQueue> pd3dCommandQueue;
	ComPtr<IDXGISwapChain3> pd3dSwapChain;
	ComPtr<ID3D12DescriptorHeap> prtvDescHeap;
	UINT rtvDescriptorSize = 0;
	ComPtr<ID3D12Resource> pRenderTarget[2];
	ComPtr<ID3D12CommandAllocator> pd3dCommandAllocator;
	ComPtr<ID3D12RootSignature> rootSign;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12Resource> pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	std::unique_ptr<CD3DX12_VIEWPORT> pViewport;
	std::unique_ptr<CD3DX12_RECT> pScissorRect;
	ComPtr<ID3D12Fence> pFence;
	UINT64 fenceValue = 0;
	HANDLE hfenceEvent = NULL;
	UINT frameCnt = 2;
	UINT frameIndex = 0;
};