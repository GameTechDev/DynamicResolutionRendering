/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or imlied.
// See the License for the specific language governing permissions and
// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "SDKMeshExt.h"
#include "DXUTmisc.h"
#include "DXUTcamera.h"
#include "SceneDescription.h"

// Forward declarations
class ModelContainer;
class CinematicCamera;

//--------------------------------------------------------------------------------------
// Basic Scene handling - 
// implements simple forward renderer with per pixel velocity and colour output.
// The screen space velocity buffer output is suitable for using in post process
// motion blur.
//--------------------------------------------------------------------------------------
class Scene
{
public:
	Scene();

	~Scene();

    HRESULT OnD3D11CreateDevice(ID3D11Device* pD3DDevice, ID3D11DeviceContext* pImmediateContext);
    void    OnD3D11ReleasingSwapChain();
    void    OnD3D11DestroyDevice();

	HRESULT OnD3D11ResizedSwapChain(	ID3D11Device* pD3DDevice,
												IDXGISwapChain* pSwapChain,
												const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc );

	void	HandleMessages( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	void    OnFrameMove( double fTime, float fElapsedTime, int RT );
	void    OnFramePaused(int RT );
	
	void    RenderScene( ID3D11Device* pD3DDevice, ID3D11DeviceContext* pD3DImmediateContext, const D3DXVECTOR2* pJitter );
	void SetShowCameras( bool bValue )
	{
		m_bShowCameras = bValue;
	}
	bool GetShowCameras() const
	{
		return m_bShowCameras;
	}
	UINT GetNumCameras() const
	{
		return m_NumCameras;
	}
	UINT GetCurrentCamera() const
	{
		return m_CurrentCamera;
	}
	const wchar_t* GetCameraName( UINT camera ) const;
	void SetCamera( UINT camera );

	int							m_CurrentFrameIndex;
	int							m_TrackIndex[2];

private:
	UINT GetCameraModel( UINT camera ) const;

	// Model related
	SceneDescription			m_SceneDesc;
	UINT						m_NumModels;
	ModelContainer*				m_pModels;

	D3DXMATRIX					m_mCenter;
	D3DXVECTOR3					m_Center; 
	D3DXVECTOR3					m_Extents;

	ID3D11InputLayout*          m_pVertexLayout;

	// Camera and light related
	CinematicCamera*			m_pCinematicCamera;
	UINT						m_NumCameras;
	UINT						m_CurrentCamera;
	bool						m_bShowCameras;

	// Shader related
	ID3D11VertexShader*         m_pVertexShader;
	ID3D11PixelShader*          m_pPixelShader;

	ID3D11ShaderResourceView*   m_pDiffuseTextureSRV;
	ID3D11ShaderResourceView*   m_pSpecularTextureSRV;
	ID3D11ShaderResourceView*   m_pNormalTextureSRV;

	ID3D11SamplerState*         m_pSamplerLinear;
	ID3D11SamplerState*         m_pSamplerLinearMipOffset;
	ID3D11BlendState*			m_pBlendStateDecal;
	ID3D11DepthStencilState*	m_pDepthStencilStateDecal;


	UINT                        m_uCBVSPerObjectBind;
	UINT                        m_uCBPSPerFrameBind;

	ID3D11Buffer*               m_pCBVSPerObject;
	ID3D11Buffer*               m_pCBPSPerModel;


	float						m_ViewportWidth;
	float						m_ViewportHeight;

private:
	//prevent assign and copy
	Scene( const Scene& rhs );
	Scene& operator=( const Scene& rhs );

};