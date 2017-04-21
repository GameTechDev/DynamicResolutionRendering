/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////

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