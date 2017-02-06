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

#pragma once

#include "DXUT.h"

struct ZoomBoxGSConstants
{
	D3DXVECTOR4 ZoomCenterPos;
};

class ZoomBox
{
public:
    ZoomBox();
    virtual ~ZoomBox();

    void SetZoomCenterPosition(FLOAT x, FLOAT y);

    HRESULT OnD3D11CreateDevice(ID3D11Device* pD3DDevice);
    HRESULT OnD3D11ResizedSwapChain(ID3D11Device* pD3DDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc);
    void    OnD3D11ReleasingSwapChain();
    void    OnD3D11DestroyDevice();
    void    OnD3D11FrameRender(
        ID3D11Device* pD3DDevice, 
        ID3D11DeviceContext* pD3DImmediateContext,
        ID3D11RenderTargetView* pRTV,
        ID3D11Resource* pRT );

private:
    ID3D11Texture2D*            m_pZoomT;
    ID3D11ShaderResourceView*   m_pZoomSRV;
	ID3D11VertexShader*         m_pVertexShader;
    ID3D11GeometryShader*       m_pGeometryShaderZoom;
    ID3D11GeometryShader*       m_pGeometryShaderBorder;
	ID3D11PixelShader*          m_pPixelShaderZoom;
    ID3D11PixelShader*          m_pPixelShaderBorder;
	ID3D11InputLayout*          m_pVertexLayout;
    ID3D11DepthStencilState*    m_pDepthStencilState;
    ID3D11BlendState*           m_pBlendState;
	ID3D11SamplerState*         m_pSamplerState;
    ID3D11Buffer*               m_pVertexBuffer;
    ID3D11Buffer*               m_pGeometryCB;
    D3DXVECTOR4                 m_vZoomCenterPos;
};