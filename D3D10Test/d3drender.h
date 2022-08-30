#pragma once
#include "framework.h"
#include "d3dx10.h"

class D3D10Render {
public:
	D3D10Render() {}
	void Setup(HWND hWnd);
	void Draw();
	void Release();
private:
	ID3D10Device* pd3dDevice = NULL;
	IDXGISwapChain* pSwapChain = NULL;
	ID3D10RenderTargetView* pRenderTargetView = NULL;
	ID3D10VertexShader* pVertexShader = NULL;
	ID3D10PixelShader* pPixelShader = NULL;
	ID3D10InputLayout* pVertexLayout = NULL;
	ID3D10Buffer* pVertexBuffer = NULL;
};