#pragma once

#include "framework.h"
#include "../dxsdk/include/d3d8.h"

class D3D8Render {
public:
	void Setup(HWND hWnd);
	void Draw();
	void Release();

private:
	IDirect3D8* gd3d8Context = NULL;
	IDirect3DDevice8* pd3d8Device = NULL;
	IDirect3DVertexBuffer8* pVertexBuffer = NULL;
};
