#include "d3drender.h"
#include <atlstr.h>

struct CUSTOMVERTEX
{
	D3DXVECTOR4 position; // The position
	D3DCOLOR    color;    // The color
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

void D3D9Render::Setup(HWND hWnd)
{
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3D9Ex)))
	{
		return;
	}

	RECT rcWnd;
	GetClientRect(hWnd, &rcWnd);

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.hDeviceWindow = hWnd;
	if (FAILED(pD3D9Ex->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
								D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, NULL, &pD3D9DeviceEx))) {
		return;
	}

	// prepare vertex buffer
	if (FAILED(pD3D9DeviceEx->CreateVertexBuffer(3 * sizeof(CUSTOMVERTEX), 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &pVertexBuffer, NULL)))
	{
		return;
	}
	CUSTOMVERTEX* pVertices;
	if (SUCCEEDED(pVertexBuffer->Lock(0, 0, (void**)&pVertices, 0)))
	{
		pVertices[0].position = D3DXVECTOR4(1200.0f, 300.0f, 1.0f, 1.0f);
		pVertices[0].color = D3DCOLOR_XRGB(255, 0, 0);
		pVertices[1].position = D3DXVECTOR4(1800.0f, 900.0f, 1.0f, 1.0f);
		pVertices[1].color = D3DCOLOR_XRGB(0, 255, 0);
		pVertices[2].position = D3DXVECTOR4(600.0f, 900.0f, 1.0f, 1.0f);
		pVertices[2].color = D3DCOLOR_XRGB(0, 0, 255);
		pVertexBuffer->Unlock();
	}

	// load image from file
	//D3DXIMAGE_INFO info;
	//D3DXGetImageInfoFromFile(L"D:\\D3DSample\\D3D9Test\\test.jpeg", &info);
	//pD3D9DeviceEx->CreateOffscreenPlainSurface(rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top, info.Format, D3DPOOL_SYSTEMMEM, &pImgFromFile, NULL);
	//D3DXLoadSurfaceFromFileW(pImgFromFile, NULL, NULL, L"D:\\D3DSample\\D3D9Test\\test.jpeg", NULL, D3DX_FILTER_NONE, 0, NULL);
}

void D3D9Render::Draw()
{
	if (pD3D9DeviceEx/* && g_pVB*/)
	{
		pD3D9DeviceEx->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

		if (SUCCEEDED(pD3D9DeviceEx->BeginScene()))
		{
			// draw triangle
			pD3D9DeviceEx->SetStreamSource(0, pVertexBuffer, 0, sizeof(CUSTOMVERTEX));
			pD3D9DeviceEx->SetFVF(D3DFVF_CUSTOMVERTEX);
			pD3D9DeviceEx->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

			// draw image
			//IDirect3DSurface9* pBackBuffer = NULL;
			//pD3D9DeviceEx->GetRenderTarget(0, &pBackBuffer);
			//pD3D9DeviceEx->UpdateSurface(pImgFromFile, NULL, pBackBuffer, NULL);
			//pBackBuffer->Release();

			pD3D9DeviceEx->EndScene();
		}

		pD3D9DeviceEx->PresentEx(0, 0, 0, 0, 0);
	}
}

void D3D9Render::Release()
{
	if (pVertexBuffer)
	{
		pVertexBuffer->Release();
	}

	if (pD3D9DeviceEx)
	{
		pD3D9DeviceEx->Release();
	}

	if (pD3D9Ex)
	{
		pD3D9Ex->Release();
	}
}
