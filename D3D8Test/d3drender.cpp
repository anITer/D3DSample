#include "d3drender.h"

struct CUSTOMVERTEX
{
	float x, y, z, rhw; // The position
	D3DCOLOR    color;    // The color
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

void D3D8Render::Setup(HWND hWnd)
{
	gd3d8Context = Direct3DCreate8(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.hDeviceWindow = hWnd;
	HRESULT hr = gd3d8Context->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pd3d8Device);

	pd3d8Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	pd3d8Device->SetRenderState(D3DRS_LIGHTING, FALSE);
	pd3d8Device->SetRenderState(D3DRS_ZENABLE, FALSE);

	// ¶¨Òå¶¥µã
	pd3d8Device->CreateVertexBuffer(3 * sizeof(CUSTOMVERTEX), 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &pVertexBuffer);
	CUSTOMVERTEX* pVertices;
	if (FAILED(pVertexBuffer->Lock(0, 0, (BYTE**)&pVertices, 0)))
	{
		return;
	}

	pVertices[0].x = 100.0f;
	pVertices[0].y = 150.0f;
	pVertices[0].z = 1.0f;
	pVertices[0].rhw = 1.0f;
	pVertices[0].color = D3DCOLOR_XRGB(255, 0, 0);

	pVertices[1].x = 250.0f;
	pVertices[1].y = 150.0f;
	pVertices[1].z = 1.0f;
	pVertices[1].rhw = 1.0f;
	pVertices[1].color = D3DCOLOR_XRGB(0, 255, 0);

	pVertices[2].x = 150.0f;
	pVertices[2].y = 250.0f;
	pVertices[2].z = 1.0f;
	pVertices[2].rhw = 1.0f;
	pVertices[2].color = D3DCOLOR_XRGB(0, 0, 255);

	pVertexBuffer->Unlock();
}

void D3D8Render::Draw()
{
	if (pd3d8Device && pVertexBuffer)
	{
		pd3d8Device->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

		pd3d8Device->BeginScene();

		HRESULT hr = pd3d8Device->SetStreamSource(0, pVertexBuffer, sizeof(CUSTOMVERTEX));
		pd3d8Device->SetVertexShader(D3DFVF_CUSTOMVERTEX);
		hr = pd3d8Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 1);

		pd3d8Device->EndScene();

		pd3d8Device->Present(0, 0, 0, 0);
	}
}

void D3D8Render::Release()
{
	if (pVertexBuffer)
	{
		pVertexBuffer->Release();
	}
	if (pd3d8Device)
	{
		pd3d8Device->Release();
	}

	if (gd3d8Context)
	{
		gd3d8Context->Release();
	}
}
