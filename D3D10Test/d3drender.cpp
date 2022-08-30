#include "d3drender.h"
#include <d3dcompiler.h>
#include <directxmath.h>

struct SimpleVertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
};

void D3D10Render::Setup(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 640;
    sd.BufferDesc.Height = 480;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice);
    if (!pSwapChain || !pd3dDevice) {
        // setup failed
        return;
    }

    // Create a render target view
    ID3D10Texture2D* pBackBuffer;
    if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBackBuffer)))
        return;
    HRESULT hr = pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return;
    pd3dDevice->OMSetRenderTargets(1, &pRenderTargetView, NULL);

    D3D10_VIEWPORT vp;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pd3dDevice->RSSetViewports(1, &vp);

    // Define the input layout
    D3D10_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = sizeof(layout) / sizeof(layout[0]);

    // Create the input layout
    ID3DBlob* blob;
    D3DReadFileToBlob(L"D:\\D3DSample\\D3D10Test\\VertexShader.cso", &blob);
    pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), &pVertexShader);
    // 创建顶点布局
    pd3dDevice->CreateInputLayout(layout, numElements,
        blob->GetBufferPointer(), blob->GetBufferSize(), &pVertexLayout);
    blob->Release();

    // Set the input layout
    pd3dDevice->IASetInputLayout(pVertexLayout);

    D3DReadFileToBlob(L"D:\\D3DSample\\D3D10Test\\PixelShader.cso", &blob);
    pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), &pPixelShader);
    blob->Release();
    pd3dDevice->VSSetShader(pVertexShader);
    pd3dDevice->PSSetShader(pPixelShader);

    // Create vertex buffer
    SimpleVertex vertices[] =
    {
        {  DirectX::XMFLOAT3(0.0f, 0.5f, 0.5f),  DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        {  DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f),  DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        {  DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f),  DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
    };
    D3D10_BUFFER_DESC bd;
    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 3;
    bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    D3D10_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = vertices;
    if (FAILED(pd3dDevice->CreateBuffer(&bd, &InitData, &pVertexBuffer)))
        return;

    // Set vertex buffer
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    pd3dDevice->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);

    // Set primitive topology
    pd3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void D3D10Render::Draw()
{
    //
    // Clear the backbuffer
    //
    if (pd3dDevice && pSwapChain)
    {
        float ClearColor[4] = { 0.0f, 0.125f, 0.6f, 1.0f }; // RGBA
        pd3dDevice->ClearRenderTargetView(pRenderTargetView, ClearColor);
        
        // Render a triangle
        pd3dDevice->Draw(3, 0);

        pSwapChain->Present(0, 0);
    }

}

void D3D10Render::Release()
{
    if (pVertexBuffer) pVertexBuffer->Release();
    if (pVertexLayout) pVertexLayout->Release();
    if (pVertexShader) pVertexShader->Release();
    if (pRenderTargetView) pRenderTargetView->Release();
    if (pd3dDevice) pd3dDevice->Release();
    if (pSwapChain) pSwapChain->Release();
}
