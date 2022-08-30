#pragma once
#include "framework.h"
#include <d3d11.h>

class D3D11Render {
public:
	D3D11Render() {}
	void Setup(HWND hWnd);
	void Draw();
	void Release();
private:
	ID3D11Device* pd3dDevice = NULL;
	IDXGISwapChain* pSwapChain = NULL;
	ID3D11DeviceContext* pd3dImmediateContext = NULL;
	ID3D11RenderTargetView* pRenderTargetView = NULL;
	ID3D11VertexShader* pVertexShader = NULL;
	ID3D11PixelShader* pPixelShader = NULL;
	ID3D11InputLayout* pVertexLayout = NULL;
	ID3D11Buffer* pVertexBuffer = NULL;
};