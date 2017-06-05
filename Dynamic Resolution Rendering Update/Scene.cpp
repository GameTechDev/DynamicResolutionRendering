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

#include "Scene.h"

#include "Utility.h"


static const float          LIGHT_RADIUS = 300.0f; // Large enough to enclose th scene



// Constant buffers
struct CB_VS_PER_OBJECT
{
    D3DXMATRIX m_mWorldViewProj;
    D3DXMATRIX m_mPrevWorldViewProj;
    D3DXMATRIX m_mWorld;
    D3DXVECTOR4 m_vEyePos;
};

struct CB_PS_PER_MODEL
{
    D3DXVECTOR4 m_vDiffuseColor;
    D3DXVECTOR4 m_vAmbientColor;
    D3DXVECTOR4 m_vSpecularColor;
    D3DXVECTOR3 m_vLightDir;
    float m_fSpecularPow;
};

//--------------------------------------------------------------------------------------
// Container for SDKMeshes and matrix information 
//--------------------------------------------------------------------------------------
class ModelContainer
{
public:
	enum Type
	{
		LIT,
		DECAL,
		UNLIT,
		CAMERA
	};

	ModelContainer()
		:  m_Type( LIT )
	{
		for(int i=0; i<3; i++)
			m_pmWorldViewProjections[i]=NULL;
	}
	~ModelContainer()
	{
		m_Mesh.Destroy();
		for(int i=0; i<3; i++)
			delete[] m_pmWorldViewProjections[i];
	}

	CDXUTSDKMeshExt             m_Mesh;
	D3DXMATRIX*					m_pmWorldViewProjections[3];
	D3DXMATRIX*					m_pmWorldCurrViewProjections;
	D3DXMATRIX*					m_pmWorldPrevViewProjections;
	Type						m_Type;

private:
	//prevent assign and copy
	ModelContainer( const ModelContainer& rhs );
	ModelContainer& operator=( const ModelContainer& rhs );

};

//--------------------------------------------------------------------------------------
// Simple derived camera class enabling the camera view to be set from an external
// view matrix.
//--------------------------------------------------------------------------------------
class CinematicCamera : public CFirstPersonCamera
{
public:
	CinematicCamera()
		: m_ModelNum( 0 )
		, m_FrameNum( 0 )
		, m_bValidModel( false )

	{
	}
	UINT m_ModelNum;
	UINT m_FrameNum;
	bool m_bValidModel;

	void SetViewMatrix( const D3DXMATRIX* pView )
	{
		m_mView = *pView;
	}
};


//--------------------------------------------------------------------------------------
// Scene Ctor
//--------------------------------------------------------------------------------------
Scene::Scene()
	: m_pVertexLayout( NULL )
	, m_pVertexShader( NULL )
	, m_pPixelShader( NULL )
	, m_pDiffuseTextureSRV( NULL )
	, m_pSpecularTextureSRV( NULL )
	, m_pNormalTextureSRV( NULL )
	, m_pSamplerLinear( NULL )
	, m_pSamplerLinearMipOffset( NULL )
	, m_uCBVSPerObjectBind( 0 )
	, m_uCBPSPerFrameBind( 0 )
	, m_pCBVSPerObject( NULL )
	, m_pCBPSPerModel( NULL )
	, m_ViewportWidth( 1280.0f )
	, m_ViewportHeight( 800.0f )
	, m_Center(0.0f, 0.0f, 0.0f)
	, m_Extents(0.0f, 0.0f, 0.0f)
	, m_pModels( NULL )
	, m_NumModels( 0 )
	, m_pCinematicCamera( new CinematicCamera )
	, m_bShowCameras( false )
	, m_pBlendStateDecal( NULL )
	, m_pDepthStencilStateDecal( NULL )
	, m_NumCameras( 1 )
	, m_CurrentCamera( 0 )


{
	D3DXMatrixIdentity( &m_mCenter );
}

//--------------------------------------------------------------------------------------
// Scene Dtor
//--------------------------------------------------------------------------------------
Scene::~Scene()
{
	OnD3D11DestroyDevice();
	delete m_pCinematicCamera;
}

//--------------------------------------------------------------------------------------
// D3DDevice dependant initialization
//--------------------------------------------------------------------------------------
HRESULT Scene::OnD3D11CreateDevice(ID3D11Device* pD3DDevice, ID3D11DeviceContext* pImmediateContext)
{
	HRESULT hr = S_OK;

	// Load scene description
	m_SceneDesc.LoadFromFile( L"scene_description.txt" );

    // Load model and textures resources
	delete[] m_pModels;
	m_NumModels = m_SceneDesc.GetNumModels();
	m_pModels = new ModelContainer[m_NumModels];
	for( UINT model = 0; model < m_NumModels; ++model )
	{
		const wchar_t* pModelFilename = m_SceneDesc.GetModelPath( model );
		if( pModelFilename )
		{
			m_pModels[model].m_Mesh.Create( pD3DDevice, pModelFilename, true );
		}
		//need to cast away const due to SDKMesh loadanimation 
		wchar_t* pAnimationPath = (wchar_t*)m_SceneDesc.GetAnimationPath( model );
		if( pAnimationPath )
		{
			m_pModels[model].m_Mesh.LoadAnimation( pAnimationPath );
		}

		const wchar_t* pParams = m_SceneDesc.GetParams( model );
		if( pParams )
		{
			if( 0 == wcscmp( pParams, L"LIT" ) )
			{
				m_pModels[model].m_Type = ModelContainer::LIT;
			}
			else if( 0 == wcscmp( pParams, L"DECAL" ) )
			{
				m_pModels[model].m_Type = ModelContainer::DECAL;
			}
			else if( 0 == wcscmp( pParams, L"UNLIT" ) )
			{
				m_pModels[model].m_Type = ModelContainer::UNLIT;
			}
			else if( 0 == wcscmp( pParams, L"CAMERA" ) )
			{
				++m_NumCameras;
				m_pModels[model].m_Type = ModelContainer::CAMERA;
			}
		}
	}


	//calculate scene center
	float		numMeshes = 0.0f;
	m_Center = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	for( UINT model = 0; model < m_NumModels; ++model )
	{

		// transform model to get matrices set up for calculating center and extents
		D3DXMATRIX identity;
		D3DXMatrixIdentity( &identity );
		m_pModels[model].m_Mesh.TransformMeshWithInterpolation( &identity, 0.0f );

		//check for same matrices for meshes
		float epsilon = 0.000001f;
		m_pModels[model].m_Mesh.CheckForRedundantMatrices( epsilon );

		for( UINT frame = 0; frame < m_pModels[model].m_Mesh.GetNumFrames(); ++frame )
		{
			// Camera world view matrices
			UINT mesh = m_pModels[model].m_Mesh.GetFrame( frame )->Mesh;
			if( INVALID_MESH != mesh )
			{
				D3DXVECTOR3 center = m_pModels[model].m_Mesh.GetMeshBBoxCenter( mesh );
				const D3DXMATRIX* pMat = m_pModels[model].m_Mesh.GetWorldMatrix( frame );
				D3DXVec3TransformCoord( &center, &center, pMat );
				m_Center += center;
				numMeshes += 1.0f;
			}
		}
	}
	m_Center /= numMeshes;

	//calculate scene extents
	m_Extents = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
	for( UINT model = 0; model < m_NumModels; ++model )
	{
		for( UINT frame = 0; frame < m_pModels[model].m_Mesh.GetNumFrames(); ++frame )
		{
			// Camera world view matrices
			UINT mesh = m_pModels[model].m_Mesh.GetFrame( frame )->Mesh;
			if( INVALID_MESH != mesh )
			{
				D3DXVECTOR3 extent = m_pModels[model].m_Mesh.GetMeshBBoxExtents( mesh );
				const D3DXMATRIX* pMat = m_pModels[model].m_Mesh.GetWorldMatrix( frame );
				D3DXVec3TransformNormal( &extent, &extent, pMat );
				D3DXVECTOR3 center = m_pModels[model].m_Mesh.GetMeshBBoxCenter( mesh );
				D3DXVec3TransformCoord( &center, &center, pMat );

				//test 'positive' extent
				D3DXVECTOR3 test = center - m_Center + extent;
				if( test.x > m_Extents.x )
				{
					m_Extents.x = test.x;
				}
				if( test.y > m_Extents.y )
				{
					m_Extents.y = test.y;
				}
				if( test.z > m_Extents.z )
				{
					m_Extents.z = test.z;
				}

				//test 'negative' extent
				test = center - m_Center - extent;
				if( test.x > m_Extents.x )
				{
					m_Extents.x = test.x;
				}
				if( test.y > m_Extents.y )
				{
					m_Extents.y = test.y;
				}
				if( test.z > m_Extents.z )
				{
					m_Extents.z = test.z;
				}
			}
		}
	}

    // Initialize the translation matrix to reposition the mesh
    D3DXMatrixTranslation( &m_mCenter, -m_Center.x, -m_Center.y, -m_Center.z );
	for( UINT model = 0; model < m_NumModels; ++model )
	{
		m_pModels[model].m_Mesh.TransformMeshWithInterpolation( &m_mCenter, 0.0f );
	}



	// Camera setup
    // Setup the camera's view parameters
    D3DXVECTOR3 vEye( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vAt( 0.0f, 0.0f, -10.0f );
    m_pCinematicCamera->SetViewParams( &vEye, &vAt );
    m_pCinematicCamera->SetRotateButtons( true, false, false );
    m_pCinematicCamera->SetScalers( 0.01f, 10000.0f );
    m_pCinematicCamera->SetDrag( true );
    m_pCinematicCamera->SetEnableYAxisMovement( true );

     // Setup the camera's view parameters
    D3DXVECTOR3 clipMin = m_Center - m_Extents;
    D3DXVECTOR3 clipMax = m_Center + m_Extents;
	m_pCinematicCamera->SetClipToBoundary( false, &clipMin, &clipMax );
    m_pCinematicCamera->FrameMove( 0 );
	m_pCinematicCamera->SetNumberOfFramesToSmoothMouseData( 2 );


	for( UINT model = 0; model < m_NumModels; ++model )
	{
		UINT numFrames = m_pModels[model].m_Mesh.GetNumFrames();
		for(int i=0; i<3; i++)
		{
			delete[] m_pModels[model].m_pmWorldViewProjections[i];
			m_pModels[model].m_pmWorldViewProjections[i] = new D3DXMATRIX[ numFrames ];
		}
	}

	// Load textures (TODO: wrong textures at this point)
    V_RETURN( D3DX11CreateShaderResourceViewFromFile( pD3DDevice, L"media\\white.png", NULL, NULL, &m_pDiffuseTextureSRV, NULL ) );
	V_RETURN( D3DX11CreateShaderResourceViewFromFile( pD3DDevice, L"media\\flat_normal.png", NULL, NULL, &m_pNormalTextureSRV, NULL ) );
    V_RETURN( D3DX11CreateShaderResourceViewFromFile( pD3DDevice, L"media\\black.png", NULL, NULL, &m_pSpecularTextureSRV, NULL ) );

    // Create sampler state
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory( &samplerDesc, sizeof( samplerDesc ) );
    samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = 16;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    V_RETURN( pD3DDevice->CreateSamplerState( &samplerDesc, &m_pSamplerLinear ) );
	samplerDesc.MipLODBias = -0.5f;	//offset mipmaps by -0.5, i.e. half a mip more detailed
	V_RETURN( pD3DDevice->CreateSamplerState( &samplerDesc, &m_pSamplerLinearMipOffset ) );

    // Compile pixel shader
 	V_RETURN( Utility::CreatePixelShaderFromFile( L"SceneShaders.hlsl", "PSMain", "ps_4_0", pD3DDevice, &m_pPixelShader ) );

    // Setup the input layout
    D3D11_INPUT_ELEMENT_DESC InputLayout[] =
    { { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "NORMAL", 0,
            DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "TEXCOORD", 0,
            DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "TANGENT", 0,
            DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }, { "BINORMAL", 0,
            DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 } };
 
	//compile vertex shader and create input layout
	Utility::CreateVertexShaderAndInputLayoutFromFile( L"SceneShaders.hlsl", "VSMain", "vs_4_0", pD3DDevice,
														&m_pVertexShader, InputLayout, ARRAYSIZE( InputLayout ), &m_pVertexLayout );

    // Constant buffers
    D3D11_BUFFER_DESC BufferDesc;
    ZeroMemory( &BufferDesc, sizeof( BufferDesc ) );
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = 0;

    // VS per object
    BufferDesc.ByteWidth = sizeof( CB_VS_PER_OBJECT );
    V_RETURN( pD3DDevice->CreateBuffer( &BufferDesc, NULL, &m_pCBVSPerObject ) );
    DXUT_SetDebugName( m_pCBVSPerObject, "CB_VS_PER_OBJECT" );

    // PS per frame
    BufferDesc.ByteWidth = sizeof( CB_PS_PER_MODEL );
    V_RETURN( pD3DDevice->CreateBuffer( &BufferDesc, NULL, &m_pCBPSPerModel ) );
    DXUT_SetDebugName( m_pCBPSPerModel, "CB_PS_PER_MODEL" );

	//blend state for decals - blend colour but not velocity (write that out)
	D3D11_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable					= FALSE;
	blendDesc.IndependentBlendEnable				= TRUE;
	blendDesc.RenderTarget[0].BlendEnable			= TRUE;
	blendDesc.RenderTarget[0].DestBlend				= D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlend				= D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha		= D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].SrcBlendAlpha			= D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp				= D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].BlendOpAlpha			= D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//set no blend for RTs other than 0, and also no write out since decals lie on geometry which has already had velocity info output
	blendDesc.RenderTarget[1]						= blendDesc.RenderTarget[0]; 
	blendDesc.RenderTarget[1].RenderTargetWriteMask = 0;
	blendDesc.RenderTarget[1].BlendEnable			= FALSE;
	blendDesc.RenderTarget[1].DestBlend				= D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[1].SrcBlend				= D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[2]						= blendDesc.RenderTarget[1]; 
	blendDesc.RenderTarget[3]						= blendDesc.RenderTarget[1]; 
	blendDesc.RenderTarget[4]						= blendDesc.RenderTarget[1]; 
	blendDesc.RenderTarget[5]						= blendDesc.RenderTarget[1]; 
	blendDesc.RenderTarget[6]						= blendDesc.RenderTarget[1]; 
	blendDesc.RenderTarget[7]						= blendDesc.RenderTarget[1]; 
	V_RETURN( pD3DDevice->CreateBlendState( &blendDesc, &m_pBlendStateDecal ) );

	//depth stencil state for decals
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	memset( &depthStencilDesc, 0, sizeof( depthStencilDesc ) );
	//only care about depth state
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;	//we don't write out Z during decal drawing
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;		//OK with co-planar decals
	depthStencilDesc.DepthEnable = TRUE;

	//depthStencilDesc.BackFace
	V_RETURN( pD3DDevice->CreateDepthStencilState( &depthStencilDesc, &m_pDepthStencilStateDecal ) );

	return hr;
}

//--------------------------------------------------------------------------------------
// D3DDevice dependant swap chain releasing
//--------------------------------------------------------------------------------------
void    Scene::OnD3D11ReleasingSwapChain()
{
}

//--------------------------------------------------------------------------------------
// D3DDevice dependant destruction
//--------------------------------------------------------------------------------------
void    Scene::OnD3D11DestroyDevice()
{
	delete[] m_pModels;
	m_pModels = NULL;

	//reset cameras
	m_NumCameras = 1;
	SetCamera( 0 );

    SAFE_RELEASE( m_pVertexLayout );

    SAFE_RELEASE( m_pDiffuseTextureSRV );
    SAFE_RELEASE( m_pNormalTextureSRV );
    SAFE_RELEASE( m_pSpecularTextureSRV );

    SAFE_RELEASE( m_pVertexShader );
    SAFE_RELEASE( m_pPixelShader );
    SAFE_RELEASE( m_pSamplerLinear );
	SAFE_RELEASE( m_pSamplerLinearMipOffset );
	SAFE_RELEASE( m_pBlendStateDecal );
	SAFE_RELEASE( m_pDepthStencilStateDecal );


    SAFE_RELEASE( m_pCBVSPerObject );
    SAFE_RELEASE( m_pCBPSPerModel );


}

//--------------------------------------------------------------------------------------
// D3DDevice dependant swap chain resized
//--------------------------------------------------------------------------------------
HRESULT Scene::OnD3D11ResizedSwapChain( ID3D11Device* pD3DDevice,
                                 IDXGISwapChain* pSwapChain,
                                 const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc )
{
	HRESULT hr = S_OK;

	m_CurrentFrameIndex = 0;


    //Camera update
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( float )pBackBufferSurfaceDesc->Height;
    m_pCinematicCamera->SetProjParams( D3DX_PI / 4.0f, fAspectRatio, 20.0f, 400000.0f ); // setting near/ far planes TODO Make dependant on model sizes

	//ensure prev worldviewposition updated to prevent motion blur smear on first frame
	D3DXMATRIX mViewProj;
    mViewProj = *m_pCinematicCamera->GetViewMatrix() * *m_pCinematicCamera->GetProjMatrix();
	for( UINT model = 0; model < m_NumModels; ++model )
	{
		m_pModels[model].m_Mesh.TransformMeshWithInterpolation( &m_mCenter, 0.0f );

		for( UINT frame = 0; frame < m_pModels[model].m_Mesh.GetNumFrames(); ++frame )
		{
			const D3DXMATRIX* pMat = m_pModels[model].m_Mesh.GetWorldMatrix( frame );

			m_pModels[model].m_pmWorldCurrViewProjections = m_pModels[model].m_pmWorldViewProjections[m_CurrentFrameIndex];
			m_pModels[model].m_pmWorldPrevViewProjections = m_pModels[model].m_pmWorldViewProjections[m_CurrentFrameIndex+1];

			m_pModels[model].m_pmWorldCurrViewProjections[ frame ] = *pMat * mViewProj;
			m_pModels[model].m_pmWorldPrevViewProjections[ frame ] = m_pModels[model].m_pmWorldCurrViewProjections[ frame ];

		}
	}

	m_ViewportWidth = (float)pBackBufferSurfaceDesc->Width;
	m_ViewportHeight = (float)pBackBufferSurfaceDesc->Height;

	return hr;
}

//--------------------------------------------------------------------------------------
// Message handling
//--------------------------------------------------------------------------------------
void	Scene::HandleMessages( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    // Camera message handlig
    m_pCinematicCamera->HandleMessages( hWnd, uMsg, wParam, lParam );

}

//--------------------------------------------------------------------------------------
// Alternative Frame move when paused
//--------------------------------------------------------------------------------------
void    Scene::OnFramePaused( int RT )
{
	int CurrentFrameIndex = m_TrackIndex[RT];
	int PrevFrameIndex = (CurrentFrameIndex+2)%3;


	for( UINT model = 0; model < m_NumModels; ++model )
	{
		m_pModels[model].m_pmWorldCurrViewProjections = m_pModels[model].m_pmWorldViewProjections[CurrentFrameIndex];
		m_pModels[model].m_pmWorldPrevViewProjections = m_pModels[model].m_pmWorldViewProjections[PrevFrameIndex];
	}
}

//--------------------------------------------------------------------------------------
// Frame move - not updated when scene paused
//--------------------------------------------------------------------------------------
void    Scene::OnFrameMove( double fTime, float fElapsedTime, int RT )
{

	int PrevFrameIndex = m_CurrentFrameIndex;
	m_CurrentFrameIndex = (m_CurrentFrameIndex+1)%3;

	m_TrackIndex[RT] = m_CurrentFrameIndex;


	//transform all models first
	for( UINT model = 0; model < m_NumModels; ++model )
	{
		m_pModels[model].m_Mesh.TransformMeshWithInterpolation( &m_mCenter, fTime );
	}

	//cinematic camera
	if( m_pCinematicCamera->m_bValidModel )
	{
		const D3DXMATRIX* pWorld = m_pModels[m_pCinematicCamera->m_ModelNum].m_Mesh.GetWorldMatrix( m_pCinematicCamera->m_FrameNum );
		 D3DXMATRIX View;
		//set camera world matrix from this.
		D3DXMatrixInverse( &View, NULL, pWorld );
		m_pCinematicCamera->SetViewMatrix( &View );
	}
	else
	{
		  m_pCinematicCamera->FrameMove( fElapsedTime );
	}

	//update model matrices
	D3DXMATRIX mViewProj;
    mViewProj = *m_pCinematicCamera->GetViewMatrix() * *m_pCinematicCamera->GetProjMatrix();
	for( UINT model = 0; model < m_NumModels; ++model )
	{
		m_pModels[model].m_pmWorldCurrViewProjections = m_pModels[model].m_pmWorldViewProjections[m_CurrentFrameIndex];
		m_pModels[model].m_pmWorldPrevViewProjections = m_pModels[model].m_pmWorldViewProjections[PrevFrameIndex];
		
		for( UINT frame = 0; frame < m_pModels[model].m_Mesh.GetNumFrames(); ++frame )
		{
			//now do actual curr update
			const D3DXMATRIX* pMat = m_pModels[model].m_Mesh.GetWorldMatrix( frame );
			m_pModels[model].m_pmWorldCurrViewProjections[ frame ] = *pMat * mViewProj;
		}
	}

}

//--------------------------------------------------------------------------------------
// Render Scene
//--------------------------------------------------------------------------------------
void    Scene::RenderScene( ID3D11Device* pD3DDevice, ID3D11DeviceContext* pD3DImmediateContext, const D3DXVECTOR2* pJitter )
{
	HRESULT hr = S_OK;

    // Set shaders - we use the same shaders for all model types, no complex materials
    pD3DImmediateContext->VSSetShader( m_pVertexShader, NULL, 0 );
    pD3DImmediateContext->PSSetShader( m_pPixelShader, NULL, 0 );

	// Loop through models
	for( UINT model = 0; model < m_NumModels; ++model )
	{
		ModelContainer& rModel = m_pModels[model];
		if( !m_bShowCameras && ModelContainer::CAMERA == rModel.m_Type )
		{
			continue;
		}
		// Update per model constant buffers
		// Since the sample uses few models, we use this per model rather than per frame to alter parameters
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		V( pD3DImmediateContext->Map( m_pCBPSPerModel, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
		CB_PS_PER_MODEL* pPerModel = ( CB_PS_PER_MODEL* )MappedResource.pData;
		if( ModelContainer::UNLIT == rModel.m_Type )
		{
			//for unlit geometry we set specular and diffuse lighting to 0.0 and ambeint to 1.0
			pPerModel->m_vDiffuseColor = D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 1.0f );
			pPerModel->m_vAmbientColor = D3DXVECTOR4( 1.0f, 1.0f, 1.0f, 1.0f );
			pPerModel->m_vSpecularColor = D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 1.0f );
		}
		else
		{
			// Lit geometry lighting values
			pPerModel->m_vDiffuseColor = D3DXVECTOR4( 1.58f, 1.36f, 0.74f, 1.0f  );
			pPerModel->m_vAmbientColor = D3DXVECTOR4( 0.42f, 0.44f, 0.46f, 1.0f );
			pPerModel->m_vSpecularColor = D3DXVECTOR4( 1.30f, 1.18f, 0.78f, 1.0f );
		}
	    D3DXVECTOR3 vLightDir( 0.89f, -0.45f, 0.089f );
		pPerModel->m_vLightDir = vLightDir;
		pPerModel->m_fSpecularPow = 16.0f;
		pD3DImmediateContext->Unmap( m_pCBPSPerModel, 0 );
		pD3DImmediateContext->PSSetConstantBuffers( m_uCBPSPerFrameBind, 1, &m_pCBPSPerModel );

		// blend and depth stencil states
		float blendFactor[4];
		memset( blendFactor, 0, sizeof( blendFactor ) );
		if( ModelContainer::DECAL == rModel.m_Type )
		{
			pD3DImmediateContext->OMSetBlendState( m_pBlendStateDecal, blendFactor, 0xffffffff );
			pD3DImmediateContext->OMSetDepthStencilState( m_pDepthStencilStateDecal, 0 );
		}
		else
		{
			pD3DImmediateContext->OMSetBlendState( NULL, blendFactor, 0xffffffff );
			pD3DImmediateContext->OMSetDepthStencilState( NULL, 0 );
		}


		int lastSetMatrixFrame = -1;
		for( UINT frame = 0; frame < rModel.m_Mesh.GetNumFrames(); ++frame )
		{
			CDXUTSDKMeshExt& rMesh = rModel.m_Mesh;
			UINT meshIndex = rMesh.GetFrame( frame )->Mesh;
			if( INVALID_MESH == meshIndex )
			{
				continue;
			}


			// Camera world view matrices
			D3DXMATRIX mWorld, mView;
			mWorld = *rMesh.GetWorldMatrix( frame );
			mView = *m_pCinematicCamera->GetViewMatrix();
			D3DXMATRIX mWorldView;
			mWorldView = mWorld * mView;

			// basic check that the mesh isn't otuside view
			D3DXVECTOR3 meshCenterOrig = rMesh.GetMeshBBoxCenter( meshIndex );
			D3DXVECTOR3 meshCenter;
			D3DXVec3TransformCoord(  &meshCenter, &meshCenterOrig, &mWorldView );
			if( meshCenter.z < 0.0f )
			{
				//center is behind view, check extents
				D3DXVECTOR3 meshExtentsOrig = rMesh.GetMeshBBoxExtents( meshIndex );
				D3DXVECTOR3 meshExtents;
				//transform in case of scale
				D3DXVec3TransformNormal( &meshExtents, &meshExtentsOrig, &mWorld );
				float fRadius = sqrt( D3DXVec3Dot( &meshExtents, &meshExtents ) );

				if( fRadius + meshCenter.z < 0.0f )
				{
					//whole mesh is behind view, continue
					continue;
				}
				else
				{
					D3DXMATRIX mWorldViewProj  = rModel.m_pmWorldCurrViewProjections[ frame ];
					D3DXVECTOR3 meshCenterProj;
					D3DXVec3TransformCoord(  &meshCenterProj, &meshCenterOrig, &mWorldViewProj );
					D3DXVECTOR3 meshExtents = rMesh.GetMeshBBoxExtents( meshIndex );
					D3DXVec3TransformNormal( &meshExtents, &meshExtentsOrig, &mWorldViewProj );
					float fRadius = sqrt( D3DXVec3Dot( &meshExtents, &meshExtents ) );
					if( meshCenterProj.x + fRadius < -1.0f ||
						meshCenterProj.y + fRadius < -1.0f ||
						meshCenterProj.x - fRadius > 1.0f ||
						meshCenterProj.y - fRadius > 1.0f )
					{
						//mesh is outside of view frustrum
						continue;
					}
				}
			}

			// Constant Buffers: VS Per object
			int thisFramesEffectiveMatrixFrame = (int)rMesh.GetFrameOfMatrix( frame );
			if( lastSetMatrixFrame != thisFramesEffectiveMatrixFrame )
			{
				lastSetMatrixFrame = thisFramesEffectiveMatrixFrame;
				V( pD3DImmediateContext->Map( m_pCBVSPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
				CB_VS_PER_OBJECT* pVSPerObject = ( CB_VS_PER_OBJECT* )MappedResource.pData;
				if( pJitter )
				{
					//have a jitter vector, so jitter both matrices to prevent motion blur smearing of jitter
					D3DXMATRIX jitterMatrix;
					D3DXMatrixTranslation( &jitterMatrix, 2.0f*pJitter->x, 2.0f*pJitter->y, 0.0f );
					D3DXMATRIX jitteredMatrix = rModel.m_pmWorldCurrViewProjections[ frame ] * jitterMatrix;
					D3DXMatrixTranspose( &pVSPerObject->m_mWorldViewProj, &jitteredMatrix );
					jitteredMatrix = rModel.m_pmWorldPrevViewProjections[ frame ] * jitterMatrix;
					D3DXMatrixTranspose( &pVSPerObject->m_mPrevWorldViewProj, &jitteredMatrix );
				}
				else
				{
						D3DXMatrixTranspose( &pVSPerObject->m_mWorldViewProj, &rModel.m_pmWorldCurrViewProjections[ frame ] );
						D3DXMatrixTranspose( &pVSPerObject->m_mPrevWorldViewProj, &rModel.m_pmWorldPrevViewProjections[ frame ]  );
				}
				D3DXMatrixTranspose( &pVSPerObject->m_mWorld, &mWorld );
				pVSPerObject->m_vEyePos = D3DXVECTOR4( m_pCinematicCamera->GetEyePt()->x, m_pCinematicCamera->GetEyePt()->y, m_pCinematicCamera->GetEyePt()->z,
													   1.0f );
				pD3DImmediateContext->Unmap( m_pCBVSPerObject, 0 );
				pD3DImmediateContext->VSSetConstantBuffers( m_uCBVSPerObjectBind, 1, &m_pCBVSPerObject );
			}

			// IA setup
			pD3DImmediateContext->IASetInputLayout( m_pVertexLayout );
			UINT Strides[1];
			UINT Offsets[1];
			ID3D11Buffer* pVB[1];
			pVB[0] = rMesh.GetVB11( meshIndex, 0 );
			Strides[0] = ( UINT )rMesh.GetVertexStride( meshIndex, 0 );
			Offsets[0] = 0;
			pD3DImmediateContext->IASetVertexBuffers( 0, 1, pVB, Strides, Offsets );
			pD3DImmediateContext->IASetIndexBuffer( rMesh.GetIB11( meshIndex ), rMesh.GetIBFormat11( meshIndex ), 0 );

			if( pJitter )
			{
				// use a mip offset if we have jitter to get more detailed textures
				pD3DImmediateContext->PSSetSamplers( 0, 1, &m_pSamplerLinearMipOffset );
			}
			else
			{
				pD3DImmediateContext->PSSetSamplers( 0, 1, &m_pSamplerLinear );
			}

			// Render subsets (different materials in current frame/node)
			for( UINT subset = 0; subset < rMesh.GetNumSubsets( meshIndex ); ++subset )
			{
				// Get the subset
				SDKMESH_SUBSET* pSubset = rMesh.GetSubset( meshIndex, subset );
				D3D11_PRIMITIVE_TOPOLOGY PrimType;
				PrimType = CDXUTSDKMesh::GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
				pD3DImmediateContext->IASetPrimitiveTopology( PrimType );

				// D3D11 - material loading
				UINT material = pSubset->MaterialID;
				SDKMESH_MATERIAL* pMaterial = rMesh.GetMaterial( material );

				ID3D11ShaderResourceView* pSRV[3];
				pSRV[0] = pMaterial->pDiffuseRV11;
				if( !pSRV[0] || IsErrorResource( pSRV[0] ) ) { pSRV[0] = m_pDiffuseTextureSRV; }
				pSRV[1] = pMaterial->pNormalRV11;
				if( !pSRV[1] || IsErrorResource( pSRV[1] ) ) { pSRV[1] = m_pNormalTextureSRV; }
				pSRV[2] = pMaterial->pSpecularRV11;
				if( !pSRV[2] || IsErrorResource( pSRV[2] ) ) { pSRV[2] = m_pSpecularTextureSRV; }
				pD3DImmediateContext->PSSetShaderResources( 0, 3, pSRV );
				
				// Draw indexed
				pD3DImmediateContext->DrawIndexed( ( UINT )pSubset->IndexCount, ( UINT )pSubset->IndexStart, ( UINT )pSubset->VertexStart );
			}

		}
	}

	//reset blend state
	float blendFactor[4];
	memset( blendFactor, 0, sizeof( blendFactor ) );
	pD3DImmediateContext->OMSetBlendState( NULL, blendFactor, 0xffffffff );
	pD3DImmediateContext->OMSetDepthStencilState( NULL, 0 );

}

//--------------------------------------------------------------------------------------
// Returns index of model on which camera index N is based on
//--------------------------------------------------------------------------------------
UINT Scene::GetCameraModel( UINT camera ) const
{
	assert( 0 != camera ); //camera 0 is not a model, but first person camera

	// below not the fastest approach, but removes need to store data and this is an infrequent operation
	UINT camcount = 1;
	for( UINT model = 0; model < m_NumModels; ++model )
	{
		if( m_pModels[ model ].m_Type == ModelContainer::CAMERA )
		{
			if( camcount == camera)
			{
				return model;
			}
			++camcount;
		}
	}
	return m_NumModels;	//error return

}


//--------------------------------------------------------------------------------------
// Returns name of camera from index
//--------------------------------------------------------------------------------------
const wchar_t*	Scene::GetCameraName( UINT camera ) const
{
	if( 0 == camera )
	{
		return L"First Person";
	}
	else
	{
		UINT model = GetCameraModel( camera );
		return m_SceneDesc.GetModelFilename( model );
	}
}


//--------------------------------------------------------------------------------------
// Set current camera - 0 is first person viewpoint (mouse/keyboard controlled)
//--------------------------------------------------------------------------------------
void	Scene::SetCamera( UINT camera )
{
	if( 0 == camera )
	{
		m_pCinematicCamera->m_bValidModel = false;
	}
	else
	{
		UINT model = GetCameraModel( camera );
		if( model < m_NumModels )
		{
			m_pCinematicCamera->m_bValidModel = true;
			m_pCinematicCamera->m_ModelNum = model;
			m_pCinematicCamera->m_FrameNum = 0;	//ensure we have a fallback in case of no models
			for( UINT frame = 0; frame < m_pModels[m_pCinematicCamera->m_ModelNum].m_Mesh.GetNumFrames(); ++frame )
			{
				// Camera world view matrices
				UINT mesh = m_pModels[m_pCinematicCamera->m_ModelNum].m_Mesh.GetFrame( frame )->Mesh;
				if( INVALID_MESH != mesh )
				{
					m_pCinematicCamera->m_FrameNum = frame;
				}
			}

		}
	}

}
