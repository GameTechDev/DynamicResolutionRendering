/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#include "DXUT.h"
#include "Utility.h"
#include "SDKmisc.h"


//--------------------------------------------------------------------------------------
// Render Target create member function
//--------------------------------------------------------------------------------------
HRESULT  Utility::RenderTarget::Create( ID3D11Device* pd3dDevice , DXGI_FORMAT format, UINT width, UINT height, const CHAR* pDebugName )
{
	//ensure previous RTs released
	SafeReleaseAll();

	bool bDepthStencil = false;
	DXGI_FORMAT texFormat = format;
	DXGI_FORMAT srvFormat = format;
	UINT Bindflags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	//check if we're being asked for a depth stencil view, and set up appropriate shader resource views.
	switch( format )
	{
	case DXGI_FORMAT_D32_FLOAT:
		bDepthStencil = true;
		srvFormat = DXGI_FORMAT_R32_FLOAT;
		texFormat = DXGI_FORMAT_R32_TYPELESS;
		Bindflags = D3D11_BIND_DEPTH_STENCIL;
		break;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		bDepthStencil = true;
		srvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;	//presumes we do not want to access stencil. Create your own view if you do!
		texFormat = DXGI_FORMAT_R24G8_TYPELESS ;
		Bindflags = D3D11_BIND_DEPTH_STENCIL;
		break;
	case DXGI_FORMAT_D16_UNORM:
		bDepthStencil = true;
		srvFormat = DXGI_FORMAT_R16_UNORM;
		texFormat = DXGI_FORMAT_R16_TYPELESS;
		Bindflags = D3D11_BIND_DEPTH_STENCIL;
		break;
	default:
		break;
	}


	// Initialise descriptors
	D3D11_TEXTURE2D_DESC			TXDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	D3D11_RENDER_TARGET_VIEW_DESC	RTVDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC	DSVDesc;

	//allow depth buffer to be sampled if possible
	if(  Bindflags & D3D11_BIND_DEPTH_STENCIL )
	{
		Bindflags |= D3D11_BIND_SHADER_RESOURCE;
	}
	TXDesc.SampleDesc.Count = 1;
	TXDesc.SampleDesc.Quality = 0;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

	TXDesc.Width = width;
	TXDesc.Height = height;
    TXDesc.MipLevels = 1;
    TXDesc.ArraySize = 1;
	TXDesc.Format = texFormat;
    TXDesc.Usage = D3D11_USAGE_DEFAULT;
    TXDesc.BindFlags = Bindflags;
    TXDesc.CPUAccessFlags = 0;
    TXDesc.MiscFlags = 0;

    SRVDesc.Format = srvFormat;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = TXDesc.MipLevels;

	// Create texture resource
	HRESULT hr;
	V_RETURN( pd3dDevice->CreateTexture2D( &TXDesc, NULL, &m_pTexture ) );

	// If we're going to bind this as a shader resource, create one
	if( Bindflags & D3D11_BIND_SHADER_RESOURCE )
	{
		hr = pd3dDevice->CreateShaderResourceView( m_pTexture, &SRVDesc, &m_pTexView );
		if( S_OK != hr )
		{
			SAFE_RELEASE( m_pTexture );
			return hr;
		}
	}

	if( !bDepthStencil )
	{
		// Not a depth stencil surface, so create a render target view
		RTVDesc.Format = texFormat;
		RTVDesc.Texture2D.MipSlice = 0;
		hr = pd3dDevice->CreateRenderTargetView( m_pTexture, &RTVDesc, &m_pRTView );
		if( S_OK != hr )
		{
			SAFE_RELEASE( m_pTexture );
			SAFE_RELEASE( m_pTexView );
			return hr;
		}
	}
	else
	{
		// Depth stencil surface, create a depth stencil view
		DSVDesc.Format = format;
		DSVDesc.Texture2D.MipSlice = 0;
		DSVDesc.Flags = 0;
		hr = pd3dDevice->CreateDepthStencilView( m_pTexture, &DSVDesc, &m_pDSView );
		if( S_OK != hr )
		{
			SAFE_RELEASE( m_pTexture );
			SAFE_RELEASE( m_pTexView );
			return hr;
		}
	}

	// Set debug names if we passed them
	if( pDebugName )
	{
		DXUT_SetDebugName( m_pTexture, pDebugName );
		DXUT_SetDebugName( m_pTexView, pDebugName );
		if( !bDepthStencil )
		{
			DXUT_SetDebugName( m_pRTView, pDebugName );
		}
		else
		{
			DXUT_SetDebugName( m_pDSView, pDebugName );
		}
	}

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Find and compile the specified shader
//--------------------------------------------------------------------------------------
HRESULT Utility::CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    // find the file
    WCHAR str[MAX_PATH];
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DX11CompileFromFile( str, NULL, NULL, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
    if( FAILED(hr) )
    {
        if( pErrorBlob != NULL )
		{
			//display in Visual Studio allowing user to double click to find file & line.
			OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );

			//allow user to decide on what to do - can fix shader and retry
			int msgboxID = MessageBoxA(
				NULL,
				(char*)pErrorBlob->GetBufferPointer(),
				"Shader compilation failed",
				MB_ICONWARNING | MB_ABORTRETRYIGNORE
				);
			SAFE_RELEASE( pErrorBlob );

			switch (msgboxID)
			{
			case IDABORT:
				//exit application completely
				exit(-1);
			case IDRETRY:
				//retry by re-running compilation
				return CompileShaderFromFile( szFileName, szEntryPoint, szShaderModel, ppBlobOut );
			case IDIGNORE:
			default:
				//just continue
				break;
			}
		}
		return hr;
	}
    SAFE_RELEASE( pErrorBlob );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Find compile and create the specified Vertex shader
// ppInputLayout, pInputElementDescs can be NULL If no layout required 
//--------------------------------------------------------------------------------------
HRESULT Utility::CreateVertexShaderAndInputLayoutFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel,
														ID3D11Device* pDevice, ID3D11VertexShader** ppVertexShaderOut,
														const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs, UINT NumElements,
														ID3D11InputLayout **ppInputLayout )
{
    HRESULT hr = S_OK;
    ID3DBlob* pShaderBuffer = NULL;
    V_RETURN( Utility::CompileShaderFromFile( szFileName, szEntryPoint, szShaderModel, &pShaderBuffer ) );

    // Create the shaders
    V_RETURN( pDevice->CreateVertexShader( pShaderBuffer->GetBufferPointer(),
                                              pShaderBuffer->GetBufferSize(), NULL, ppVertexShaderOut ) );
    DXUT_SetDebugName( *ppVertexShaderOut, szEntryPoint );

	if( ppInputLayout )
	{
	    V_RETURN( pDevice->CreateInputLayout( pInputElementDescs, NumElements, pShaderBuffer->GetBufferPointer(),
                                             pShaderBuffer->GetBufferSize(), ppInputLayout ) );
	}

	SAFE_RELEASE( pShaderBuffer );
	return hr;
}

//--------------------------------------------------------------------------------------
// Find compile and create the specified Pixel shader
//--------------------------------------------------------------------------------------
HRESULT Utility::CreatePixelShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3D11Device* pDevice, ID3D11PixelShader** ppPixelShaderOut )
{
	HRESULT hr = S_OK;
	ID3DBlob* pShaderBuffer = NULL;
	V_RETURN( Utility::CompileShaderFromFile( szFileName, szEntryPoint, szShaderModel, &pShaderBuffer ) );

    // Create the shaders
    V_RETURN( pDevice->CreatePixelShader( pShaderBuffer->GetBufferPointer(),
                                              pShaderBuffer->GetBufferSize(), NULL, ppPixelShaderOut ) );
    DXUT_SetDebugName( *ppPixelShaderOut, szEntryPoint );
	SAFE_RELEASE( pShaderBuffer );
	return hr;
}
