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

#include "dxut.h"


//--------------------------------------------------------------------------------------
// Utility classes and functions
//--------------------------------------------------------------------------------------
namespace Utility
{
	//--------------------------------------------------------------------------------------
	// Render Target class for keeping render target view, shader resource view, and texture together
	//--------------------------------------------------------------------------------------
	class RenderTarget
	{
	public:
		RenderTarget()
		  : m_pRTView( NULL )
		  , m_pTexView( NULL )
		  , m_pTexture( NULL )
		  , m_pDSView( NULL )
		{
		}

		HRESULT Create( ID3D11Device* pd3dDevice , DXGI_FORMAT format, UINT width, UINT height, const CHAR* pDebugName );

		~RenderTarget()
		{
			SafeReleaseAll();
		}

		void SafeReleaseAll()
		{
			SAFE_RELEASE( m_pRTView );
			SAFE_RELEASE( m_pTexView );
			SAFE_RELEASE( m_pDSView );
			SAFE_RELEASE( m_pTexture );
		}

		ID3D11RenderTargetView*				GetRenderTargetView() const
		{
			return m_pRTView;
		}
		ID3D11ShaderResourceView*			GetShaderResourceView() const
		{
			return m_pTexView;
		}
		ID3D11DepthStencilView*				GetDepthStencilView() const
		{
			return m_pDSView;
		}
		ID3D11Texture2D*					GetTexture2D() const
		{
			return m_pTexture;
		}

	private:

		ID3D11RenderTargetView*				m_pRTView;
		ID3D11ShaderResourceView*           m_pTexView;
		ID3D11DepthStencilView*				m_pDSView;
		ID3D11Texture2D*					m_pTexture;
	};




	//--------------------------------------------------------------------------------------
	// Find and compile the specified shader
	//--------------------------------------------------------------------------------------
	HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut );

	//--------------------------------------------------------------------------------------
	// Find compile and create the specified Vertex shader
	// ppInputLayout, pInputElementDescs can be NULL If no layout required 
	//--------------------------------------------------------------------------------------
	HRESULT CreateVertexShaderAndInputLayoutFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel,
														ID3D11Device* pDevice, ID3D11VertexShader** ppVertexShaderOut,
														const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs, UINT NumElements,
														ID3D11InputLayout **ppInputLayout );

	//--------------------------------------------------------------------------------------
	// Find compile and create the specified Pixel shader
	//--------------------------------------------------------------------------------------
	HRESULT CreatePixelShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel,
											ID3D11Device* pDevice, ID3D11PixelShader** ppPixelShaderOut );
}