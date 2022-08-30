#pragma once

#include "framework.h"
#include "../dxsdk/Include/d3dx9.h"

class D3D9Render {
public:
	void Setup(HWND hWnd);
	void Draw();
	void Release();

private:
	IDirect3D9Ex* pD3D9Ex = NULL;
	IDirect3DDevice9Ex* pD3D9DeviceEx = NULL;
	IDirect3DVertexBuffer9* pVertexBuffer = NULL;
	IDirect3DSurface9* pImgFromFile = NULL;
};