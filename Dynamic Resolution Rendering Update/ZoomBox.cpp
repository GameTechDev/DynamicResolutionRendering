// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies. Intel makes no representations about the
// suitability of this software for any purpose. THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsability for any errors which may appear in this software nor any
// responsibility to update it.

#include "Utility.h"
#include "ZoomBox.h"


ZoomBox::ZoomBox() : m_pZoomT(NULL), m_pZoomSRV(NULL), m_pVertexShader(NULL), 
                     m_pGeometryShaderZoom(NULL), m_pGeometryShaderBorder(NULL),
                     m_pPixelShaderZoom(NULL), m_pPixelShaderBorder(NULL),
					 m_pVertexLayout(NULL), m_pSamplerState(NULL), m_pVertexBuffer(NULL),
                     m_vZoomCenterPos(0.5f, 0.5f, 0.f, 0.f)
{
}


ZoomBox::~ZoomBox() {}


void ZoomBox::SetZoomCenterPosition(FLOAT x, FLOAT y)
{
    m_vZoomCenterPos.x = x;
    m_vZoomCenterPos.y = y;
}


HRESULT ZoomBox::OnD3D11CreateDevice(ID3D11Device* pD3DDevice)
{
	HRESULT hr = S_OK;

	 // Create vertex input layout
	D3D11_INPUT_ELEMENT_DESC InputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Compile & create vertex shader
	Utility::CreateVertexShaderAndInputLayoutFromFile( L"ZoomBox.hlsl", "VS", "vs_4_0",
														pD3DDevice, &m_pVertexShader,
														InputLayout, ARRAYSIZE( InputLayout ), &m_pVertexLayout );

    // Compile & create geometry shaders
    ID3DBlob *pGSBlob = NULL, *pGS2Blob = NULL;


	Utility::CompileShaderFromFile( L"ZoomBox.hlsl","GS_Zoom", "gs_4_0",&pGSBlob);
    V_RETURN(pD3DDevice->CreateGeometryShader(pGSBlob->GetBufferPointer(), pGSBlob->GetBufferSize(),
                                          NULL, &m_pGeometryShaderZoom));


	Utility::CompileShaderFromFile( L"ZoomBox.hlsl","GS_Border", "gs_4_0",&pGS2Blob);
    V_RETURN(pD3DDevice->CreateGeometryShader(pGS2Blob->GetBufferPointer(), pGS2Blob->GetBufferSize(),
                                          NULL, &m_pGeometryShaderBorder));

    pGSBlob->Release();
    pGS2Blob->Release();


	// Compile & create pixel shaders
	Utility::CreatePixelShaderFromFile( L"ZoomBox.hlsl","PS_Zoom", "ps_4_0",
										pD3DDevice, &m_pPixelShaderZoom );

	Utility::CreatePixelShaderFromFile( L"ZoomBox.hlsl","PS_Border", "ps_4_0",
										pD3DDevice, &m_pPixelShaderBorder );

	// Create states
    D3D11_DEPTH_STENCIL_DESC DSDesc;
    ZeroMemory(&DSDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
    DSDesc.DepthEnable = FALSE;
    DSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    DSDesc.DepthFunc = D3D11_COMPARISON_LESS;
    DSDesc.StencilEnable = FALSE;
    V_RETURN(pD3DDevice->CreateDepthStencilState(&DSDesc, &m_pDepthStencilState));

    D3D11_BLEND_DESC BSDesc;
    ZeroMemory(&BSDesc, sizeof(D3D11_BLEND_DESC));
    BSDesc.RenderTarget[0].BlendEnable = FALSE;
    BSDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
    V_RETURN(pD3DDevice->CreateBlendState(&BSDesc, &m_pBlendState));

	D3D11_SAMPLER_DESC SamplerDesc;
	ZeroMemory(&SamplerDesc, sizeof(SamplerDesc));
	SamplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
	SamplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.MaxAnisotropy  = 1;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	SamplerDesc.MaxLOD         = D3D11_FLOAT32_MAX;
	V_RETURN(pD3DDevice->CreateSamplerState(&SamplerDesc, &m_pSamplerState));

    // Make a single vertex
    D3DXVECTOR3 Vertex = D3DXVECTOR3(1.f, 1.f, 1.f);
    D3D11_BUFFER_DESC VBDesc;
    VBDesc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
    VBDesc.ByteWidth           = sizeof(Vertex);
    VBDesc.CPUAccessFlags      = 0;
    VBDesc.MiscFlags           = 0;
    VBDesc.StructureByteStride = 0;
    VBDesc.Usage               = D3D11_USAGE_DEFAULT;
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = Vertex;
    V_RETURN(pD3DDevice->CreateBuffer(&VBDesc, &InitData, &m_pVertexBuffer));

	// Create constant buffer
	D3D11_BUFFER_DESC BufferDesc;
	ZeroMemory(&BufferDesc, sizeof(BufferDesc));
	BufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
	BufferDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.ByteWidth      = sizeof(ZoomBoxGSConstants);
	V_RETURN(pD3DDevice->CreateBuffer(&BufferDesc, NULL, &m_pGeometryCB));

	return hr;
}


HRESULT ZoomBox::OnD3D11ResizedSwapChain(ID3D11Device* pD3DDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
    D3D11_TEXTURE2D_DESC ZoomTDesc;
    ZeroMemory(&ZoomTDesc, sizeof(ZoomTDesc));
    ZoomTDesc.Width                = pBackBufferSurfaceDesc->Width;
    ZoomTDesc.Height               = pBackBufferSurfaceDesc->Height;
    ZoomTDesc.MipLevels            = 1;
    ZoomTDesc.ArraySize            = 1;
    ZoomTDesc.Format               = pBackBufferSurfaceDesc->Format;
    ZoomTDesc.SampleDesc.Count     = 1;
    ZoomTDesc.Usage                = D3D11_USAGE_DEFAULT;
    ZoomTDesc.CPUAccessFlags       = 0;
    ZoomTDesc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SHADER_RESOURCE_VIEW_DESC RSVDesc;
    ZeroMemory(&RSVDesc, sizeof(RSVDesc));
    RSVDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    RSVDesc.Format                    = pBackBufferSurfaceDesc->Format;
    RSVDesc.Texture2D.MostDetailedMip = 0;
    RSVDesc.Texture2D.MipLevels       = 1;

    HRESULT hr;
    V_RETURN(pD3DDevice->CreateTexture2D(&ZoomTDesc, NULL, &m_pZoomT));
    V_RETURN(pD3DDevice->CreateShaderResourceView(m_pZoomT, &RSVDesc, &m_pZoomSRV));

    return hr;
}


void ZoomBox::OnD3D11ReleasingSwapChain()
{
    SAFE_RELEASE(m_pZoomT);
    SAFE_RELEASE(m_pZoomSRV);
}


void ZoomBox::OnD3D11DestroyDevice()
{
    SAFE_RELEASE(m_pVertexShader);
    SAFE_RELEASE(m_pGeometryShaderZoom);
    SAFE_RELEASE(m_pGeometryShaderBorder);
    SAFE_RELEASE(m_pPixelShaderZoom);
    SAFE_RELEASE(m_pPixelShaderBorder);
    SAFE_RELEASE(m_pVertexLayout);
    SAFE_RELEASE(m_pDepthStencilState);
    SAFE_RELEASE(m_pBlendState);
	SAFE_RELEASE(m_pSamplerState);
    SAFE_RELEASE(m_pVertexBuffer);
    SAFE_RELEASE(m_pGeometryCB);
}


void ZoomBox::OnD3D11FrameRender(
    ID3D11Device* pD3DDevice, 
    ID3D11DeviceContext* pD3DImmediateContext,
    ID3D11RenderTargetView* pRTV,
    ID3D11Resource* pRT )
{

    // Copy color buffer to zoom texture
    pD3DImmediateContext->CopyResource(m_pZoomT, pRT);

    FLOAT Factors[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    pD3DImmediateContext->OMSetBlendState(m_pBlendState, Factors, 0xFFFFFFFF);
    pD3DImmediateContext->OMSetDepthStencilState(m_pDepthStencilState, 0);

	pD3DImmediateContext->VSSetShader(m_pVertexShader, NULL, 0);			
    pD3DImmediateContext->GSSetShader(m_pGeometryShaderZoom, NULL, 0);
    pD3DImmediateContext->GSSetConstantBuffers(0, 1, &m_pGeometryCB);
	pD3DImmediateContext->PSSetShader(m_pPixelShaderZoom, NULL, 0);
    pD3DImmediateContext->PSSetShaderResources(0, 1, &m_pZoomSRV);
	pD3DImmediateContext->PSSetSamplers(0, 1, &m_pSamplerState);

	D3D11_MAPPED_SUBRESOURCE MappedCB;
	pD3DImmediateContext->Map(m_pGeometryCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedCB);
	ZoomBoxGSConstants *pCB = (ZoomBoxGSConstants *) MappedCB.pData;
	pCB->ZoomCenterPos = m_vZoomCenterPos;	
	pD3DImmediateContext->Unmap(m_pGeometryCB, 0);

	pD3DImmediateContext->IASetInputLayout(m_pVertexLayout);
	pD3DImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    UINT Stride = sizeof(D3DXVECTOR3), Offset = 0;
    pD3DImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &Stride, &Offset);

    pD3DImmediateContext->Draw(1, 0);

    pD3DImmediateContext->GSSetShader(m_pGeometryShaderBorder, NULL, 0);
    pD3DImmediateContext->PSSetShader(m_pPixelShaderBorder, NULL, 0);

    pD3DImmediateContext->Draw(1, 0);
}
