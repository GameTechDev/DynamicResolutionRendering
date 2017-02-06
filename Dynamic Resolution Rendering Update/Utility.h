//--------------------------------------------------------------------------------------
// Copyright 2010 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//
//--------------------------------------------------------------------------------------
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