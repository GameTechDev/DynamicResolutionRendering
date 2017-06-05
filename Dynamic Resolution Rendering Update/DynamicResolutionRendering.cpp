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

#include "DynamicResolutionRendering.h"
#include "Scene.h"
#include "resource.h"
#include "Utility.h"
#include "TexGenUtils.h"
#include "DynamicResolution.h"
#include "GPUTimer.h"
#include "ZoomBox.h"

// Globals: GUI related
CDXUTDialogResourceManager  g_DialogResourceManager;
CD3DSettingsDlg             g_D3DSettingsDlg;
CDXUTDialog                 g_HUD;
CDXUTDialog                 g_SampleUI;
CDXUTDialog                 g_HelpUI;
CDXUTTextHelper*            g_pTextHelper = NULL;

bool                        g_bShowHelp = false; // render the help text
bool                        g_bDisplayGUI = true; // display the GUI?
bool                        g_bDisplayDemoText = true; // display screen text?

static const unsigned int   g_uGUIWidth = 200;
static const unsigned int   g_uGUIHeight = 23;
ZoomBox                     g_ZoomBox;
bool                        g_ShowZoomBox           = false;




// Globals: Sample Component related

// Constant parameters to constrain dynamic render targets. Should be made device dependant, using memory limits.
const unsigned int			g_MaxRTWidth			= 8192; // max width of dynamic RT
const unsigned int			g_MaxRTHeight			= 8192; // max height of dynamic RT
const float					g_VelocityToAlphaScale	= 16.0f;

unsigned int                g_ResolutionScaleX = 100; // % value, updated during init
unsigned int                g_ResolutionScaleY = 100; // % value, updated during init
bool						g_bDynamicResolutionEnabled = true;
RESOLVE_MODE				g_ResolveMode = RESOLVE_MODE_TEMPORALAA;
bool						g_bPaused = false;
bool						g_bMotionBlur = true;
bool						g_bShowCameras = false;
bool						g_bAspectRatioLock = true;
bool						g_bClearWithPixelShader = true;
unsigned int				g_ResolutionScaleMax	= 1;	// 1==back buffer size, > 1 means larger. Only 2 useful for super sampling without multi-downsample.
bool						g_bResetDynamicRes = false;
unsigned int				g_CameraIndex = 1;
unsigned int				g_SymmetricTAA = 0;				// 0==Asymmetric Temporal AA using offsets [0,0],[0.5,0.5], 1==symmetric TAA with [-0.25,-0.25],[+0.25,+0.25]

// Globals: Scene
Scene						g_Scene;

// Globals: Post Process
Utility::RenderTarget		g_Color;
Utility::RenderTarget		g_ColorDynamic;
Utility::RenderTarget		g_Velocity;
Utility::RenderTarget		g_VelocityDynamic[2];
Utility::RenderTarget		g_FinalRTDynamic[2];
unsigned int				g_CurrentRT = 0;
Utility::RenderTarget		g_DepthBufferDynamic;

ID3D11Buffer*				g_pFullScreenQuadBuffer = NULL;

ID3D11InputLayout*			g_pPostProcessInputLayout = NULL;
ID3D11InputLayout*			g_pPostProcessDynamicInputLayout = NULL;
ID3D11VertexShader*			g_pPostProcessVertexShader = NULL;
ID3D11PixelShader*			g_pPostProcessPixelShader = NULL;
ID3D11VertexShader*			g_pPostProcessDynamicVertexShader = NULL;
ID3D11PixelShader*			g_pPostProcessDynamicPixelShader = NULL;
ID3D11PixelShader*			g_pResolveDynamicPixelShader = NULL;
ID3D11PixelShader*			g_pResolveDynamicCubicPixelShader = NULL;
ID3D11PixelShader*			g_pResolveDynamicNoisePixelShader = NULL;
ID3D11PixelShader*			g_pResolveDynamicNoiseOffsetPixelShader = NULL;
ID3D11VertexShader*			g_pResolveTemporalAAVertexShader = NULL;
ID3D11PixelShader*			g_pResolveTemporalAAPixelShader = NULL;
ID3D11PixelShader*			g_pResolveTemporalAAFastPixelShader = NULL;
ID3D11PixelShader*			g_pResolveTemporalAANOPixelShader = NULL;
ID3D11PixelShader*			g_pResolveTemporalAANOFastPixelShader = NULL;
ID3D11PixelShader*			g_pResolveTemporalAABasicPixelShader = NULL;

ID3D11PixelShader*			g_pClearPixelShader = NULL;
ID3D11VertexShader*			g_pClearVertexShader = NULL;
ID3D11DepthStencilState*	g_pClearDepthStencilState = NULL;
ID3D11SamplerState*			g_pSamPoint = NULL;
ID3D11SamplerState*			g_pSamLinear = NULL;
ID3D11SamplerState*			g_pSamLinearWrap = NULL;
D3D11_VIEWPORT				g_ViewPort;
ID3D11Texture1D*			g_pCubicLookupFilterTex;
ID3D11ShaderResourceView*	g_pCubicLookupFilterSRV;
ID3D11ShaderResourceView*	g_pNoiseTextureSRV;

const unsigned int			g_NoiseTextureSize = 128;

// Globals: Timing
AveragedGPUTimer			g_GPUFrameInnerWorkTimer( 10 );
AveragedGPUTimer			g_GPUFrameClearTimer( 10 );
AveragedGPUTimer			g_GPUFrameSceneTimer( 10 );
AveragedGPUTimer			g_GPUFramePostProcTimer( 10 );
AveragedGPUTimer			g_GPUFrameScaleTimer( 10 );

//--------------------------------------------------------------------------------------
// Helper class used to smooth per frame timing so as to reduce hitch instabilities
//--------------------------------------------------------------------------------------
class SmoothedTimer
{
public:
	SmoothedTimer( unsigned int smoothInterval )
		: m_SmoothInterval( smoothInterval )
		, m_Time( 0.0 )
		, m_Elapsed( 0.0f )
		, m_Curr( 0 )
		, m_pDeltaTs( NULL )
		, m_fSmoothInterval( (float)smoothInterval )
	{
		m_pDeltaTs = new float[m_SmoothInterval];

		//init with 60FPS in ms
		for( unsigned int i=0; i < m_SmoothInterval; ++i )
		{
			m_pDeltaTs[i] = 1.0f/(60.0f*1000.0f);
		}

	}

	~SmoothedTimer()
	{
		delete[] m_pDeltaTs;
	}

	void Update( float deltaT )
	{
		m_pDeltaTs[ m_Curr ] = deltaT;
		++m_Curr;
		if( m_Curr >= m_SmoothInterval )
		{
			m_Curr = 0;
		}

		//to avoid potentially accumulating rounding errors we calculate all every frame
		m_Elapsed = 0.0f;
		for( unsigned int i=0; i < m_SmoothInterval; ++i )
		{
			m_Elapsed += m_pDeltaTs[i];
		}
		m_Elapsed /= m_fSmoothInterval;
		m_Time += m_Elapsed;

	}
	double GetTime() const
	{
		return m_Time;
	}
	float GetElapsedTime() const
	{
		return m_Elapsed;
	}


private:
	float*			m_pDeltaTs;
	unsigned int	m_SmoothInterval;
	float			m_fSmoothInterval;
	double			m_Time;
	float			m_Elapsed;
	unsigned int	m_Curr;

	//prevent assign and copy
	SmoothedTimer( const SmoothedTimer& rhs );
	SmoothedTimer& operator=( const SmoothedTimer& rhs );


};

SmoothedTimer g_SmoothedTimer(10);

// Globals: Dynamic resolution control
float				g_ControlledScaleMin	= 0.3f;
float				g_ControlledScale		= 1.0f;
CONTROL_MODE		g_ControlMode			= CONTROL_MODE_VSYNC;
float				g_VSyncFrameRate		= 60.0f;
DynamicResolution	g_DynamicResolution;


// Constant Buffers
struct CB_VS_POSTPROCESS
{
	D3DXVECTOR4	g_SubSampleRTCurrRatio;
};
ID3D11Buffer*				m_pCBPostProcess = NULL;

struct CB_PS_MOTIONBLUR
{
	D3DXVECTOR4	g_SubSampleRTCurrRatio;
};
ID3D11Buffer*				m_pCBMotionBlur = NULL;

struct CB_PS_RESOLVECUBIC
{
	D3DXVECTOR2	g_texsize_x;	// float2( 1/width, 0 )
	D3DXVECTOR2	g_texsize_y;	// float2( 0, 1/height )
	D3DXVECTOR4	g_SizeSource;	// float4( width, height, Not used, Not used )
};
ID3D11Buffer*				m_pCBPSResolveCubic = NULL;

struct CB_PS_RESOLVENOISE
{
	D3DXVECTOR2	g_NoiseTexScale;		// float2( scale.x, scale.y )
	D3DXVECTOR2	g_NoiseOffset;		// float2( offset.x, offset.x )
	D3DXVECTOR4	g_NoiseScale;    // float4( width, height, Not used, Not used )
};
ID3D11Buffer*				m_pCBPSResolveNoise = NULL;

struct CB_VS_POSTPROCESS_TEMPORAL_AA
{
	D3DXVECTOR2	g_VSRTCurrRatio0;
	D3DXVECTOR2	g_VSOffset0;
	D3DXVECTOR2	g_VSRTCurrRatio1;
	D3DXVECTOR2	g_VSOffset1;
};
ID3D11Buffer*				m_pCBVSPostProcessTemporalAA = NULL;
CB_VS_POSTPROCESS_TEMPORAL_AA g_TemporalAAVSConstants;

struct CB_PS_POSTPROCESS_TEMPORAL_AA
{
	D3DXVECTOR2	g_VelocityScale;
	D3DXVECTOR2	g_VelocityAlphaScale;
};
ID3D11Buffer*				m_pCBPSPostProcessTemporalAA = NULL;

//--------------------------------------------------------------------------------------
// Initlialize resources that do not depend on back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pD3DDevice,
                                      const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    HRESULT hr = S_FALSE;


    ID3D11DeviceContext* pImmediateContext = DXUTGetD3D11DeviceContext();
    V_RETURN( g_DialogResourceManager.OnD3D11CreateDevice( pD3DDevice, pImmediateContext ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11CreateDevice( pD3DDevice ) );
    g_pTextHelper = new CDXUTTextHelper( pD3DDevice, pImmediateContext, &g_DialogResourceManager, 15 );

	// init post process...



	//create depth stencil state for pixel shader clear
	D3D11_DEPTH_STENCIL_DESC depthDesc;
	memset( &depthDesc, 0, sizeof( depthDesc ) );
	depthDesc.DepthEnable = FALSE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	depthDesc.StencilEnable = FALSE;
	V_RETURN( pD3DDevice->CreateDepthStencilState( &depthDesc, &g_pClearDepthStencilState ) );
														
    // create point sampler
    D3D11_SAMPLER_DESC samDesc;
    ZeroMemory( &samDesc, sizeof(samDesc) );
    samDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
    samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samDesc.MaxAnisotropy = 1;
    samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samDesc.MaxLOD = D3D11_FLOAT32_MAX;
    V_RETURN( pD3DDevice->CreateSamplerState( &samDesc, &g_pSamPoint ) );
    DXUT_SetDebugName(g_pSamPoint, "Point" );

	// create linear sampler
    samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    V_RETURN( pD3DDevice->CreateSamplerState( &samDesc, &g_pSamLinear ) );
    DXUT_SetDebugName( g_pSamLinear, "Linear" );

	//create linear wrapped texture
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    V_RETURN( pD3DDevice->CreateSamplerState( &samDesc, &g_pSamLinearWrap ) );
    DXUT_SetDebugName( g_pSamLinear, "Linear Wrapped" );
 	

	// create fullcsreen quad vertex buffer
	D3D11_BUFFER_DESC fullScreenQuadBDesc;
    fullScreenQuadBDesc.ByteWidth = 4 * sizeof( D3DXVECTOR3 );
    fullScreenQuadBDesc.Usage = D3D11_USAGE_IMMUTABLE;
    fullScreenQuadBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    fullScreenQuadBDesc.CPUAccessFlags = 0;
    fullScreenQuadBDesc.MiscFlags = 0;
    D3DXVECTOR3 fullScreenQuadVerts[4] =
    {
        D3DXVECTOR3( -1, -1, 0.5f ),
        D3DXVECTOR3( -1, 1, 0.5f ),
        D3DXVECTOR3( 1, -1, 0.5f ),
        D3DXVECTOR3( 1, 1, 0.5f )
    };
    D3D11_SUBRESOURCE_DATA fullScreenQuadInitData;
    fullScreenQuadInitData.pSysMem = fullScreenQuadVerts;
    V_RETURN( pD3DDevice->CreateBuffer( &fullScreenQuadBDesc, &fullScreenQuadInitData, &g_pFullScreenQuadBuffer ) );


    // Constant buffers
    D3D11_BUFFER_DESC BufferDesc;
    ZeroMemory( &BufferDesc, sizeof( BufferDesc ) );
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = 0;

    // Post process CB for VS
    BufferDesc.ByteWidth = sizeof( CB_VS_POSTPROCESS );
    V_RETURN( pD3DDevice->CreateBuffer( &BufferDesc, NULL, &m_pCBPostProcess ) );
    DXUT_SetDebugName( m_pCBPostProcess, "CB_VS_POSTPROCESS" );

	// Motion Blur CB for PS
    BufferDesc.ByteWidth = sizeof( CB_PS_MOTIONBLUR );
    V_RETURN( pD3DDevice->CreateBuffer( &BufferDesc, NULL, &m_pCBMotionBlur ) );
    DXUT_SetDebugName( m_pCBMotionBlur, "CB_PS_MOTIONBLUR" );

    // Resolve Cubic CB for PS
    BufferDesc.ByteWidth = sizeof( CB_PS_RESOLVECUBIC );
	V_RETURN( pD3DDevice->CreateBuffer( &BufferDesc, NULL, &m_pCBPSResolveCubic ) );

	// Resolve Noise CB for PS
    BufferDesc.ByteWidth = sizeof( CB_PS_RESOLVENOISE );
	V_RETURN( pD3DDevice->CreateBuffer( &BufferDesc, NULL, &m_pCBPSResolveNoise ) );

	// Resolve Temporal AA for VS
    BufferDesc.ByteWidth = sizeof( CB_VS_POSTPROCESS_TEMPORAL_AA );
	V_RETURN( pD3DDevice->CreateBuffer( &BufferDesc, NULL, &m_pCBVSPostProcessTemporalAA ) );

	// Resolve Temporal AA for PS
    BufferDesc.ByteWidth = sizeof( CB_PS_POSTPROCESS_TEMPORAL_AA );
	V_RETURN( pD3DDevice->CreateBuffer( &BufferDesc, NULL, &m_pCBPSPostProcessTemporalAA ) );

	// Texture and view needed for cubic resolve
	D3D11_TEXTURE1D_DESC	desc1DCubicResolve;
	desc1DCubicResolve.ArraySize = 1;
	desc1DCubicResolve.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc1DCubicResolve.CPUAccessFlags = 0;
	desc1DCubicResolve.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc1DCubicResolve.MipLevels = 1;
	desc1DCubicResolve.MiscFlags = 0;
	desc1DCubicResolve.Usage = D3D11_USAGE_DEFAULT;
	desc1DCubicResolve.Width = 128;
	D3D11_SUBRESOURCE_DATA  dataCubicResolve;
	CubicFilterLookupTable	cubicResolveLookupTable( desc1DCubicResolve.Width );
	dataCubicResolve.pSysMem = cubicResolveLookupTable.GetTablePointer();
	dataCubicResolve.SysMemPitch = 0;		//don't need for 1D
	dataCubicResolve.SysMemSlicePitch = 0;	//don't need for 1D

	V_RETURN( pD3DDevice->CreateTexture1D( &desc1DCubicResolve, &dataCubicResolve, &g_pCubicLookupFilterTex ) );
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    SRVDesc.Format = desc1DCubicResolve.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
    SRVDesc.Texture1D.MostDetailedMip = 0;
    SRVDesc.Texture1D.MipLevels = 1;
	V_RETURN( pD3DDevice->CreateShaderResourceView( g_pCubicLookupFilterTex, &SRVDesc, &g_pCubicLookupFilterSRV ) );

	//Texture needed for noise resolve
	D3D11_TEXTURE2D_DESC desc2DNoiseTexture;
	desc2DNoiseTexture.ArraySize = 1;
	desc2DNoiseTexture.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc2DNoiseTexture.CPUAccessFlags = 0;
	desc2DNoiseTexture.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc2DNoiseTexture.Height = g_NoiseTextureSize;
	desc2DNoiseTexture.MipLevels = 1;
	desc2DNoiseTexture.MiscFlags = 0;
	desc2DNoiseTexture.SampleDesc.Count = 1;
	desc2DNoiseTexture.SampleDesc.Quality = 0;
	desc2DNoiseTexture.Usage = D3D11_USAGE_DEFAULT;
	desc2DNoiseTexture.Width = g_NoiseTextureSize;

	//generate data
	srand( 42 ); //want the same random texture every time
	ID3D11Texture2D* pNoiseTex;
	NoiseTexture noiseTexData( g_NoiseTextureSize );
	D3D11_SUBRESOURCE_DATA  data2dNoiseData;
	data2dNoiseData.pSysMem = noiseTexData.GetPointer();
	data2dNoiseData.SysMemPitch = sizeof( unsigned char ) * 4 * desc2DNoiseTexture.Width;
	data2dNoiseData.SysMemSlicePitch = 0; //do not need for 2D
	V_RETURN( pD3DDevice->CreateTexture2D( &desc2DNoiseTexture, &data2dNoiseData, &pNoiseTex ) );

	//create SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVNoiseTexDesc;
    SRVNoiseTexDesc.Format = desc2DNoiseTexture.Format;
    SRVNoiseTexDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVNoiseTexDesc.Texture1D.MostDetailedMip = 0;
    SRVNoiseTexDesc.Texture1D.MipLevels = 1;
	V_RETURN( pD3DDevice->CreateShaderResourceView( pNoiseTex, &SRVNoiseTexDesc, &g_pNoiseTextureSRV ) );
	pNoiseTex->Release();	//no longer needed.

	//init timers
	g_GPUFrameInnerWorkTimer.OnD3D11CreateDevice(  pD3DDevice, pImmediateContext );
	g_GPUFrameClearTimer.OnD3D11CreateDevice(  pD3DDevice, pImmediateContext );
	g_GPUFrameSceneTimer.OnD3D11CreateDevice(  pD3DDevice, pImmediateContext );
	g_GPUFramePostProcTimer.OnD3D11CreateDevice(  pD3DDevice, pImmediateContext );
	g_GPUFrameScaleTimer.OnD3D11CreateDevice(  pD3DDevice, pImmediateContext );
	//init scenes
	g_Scene.OnD3D11CreateDevice( pD3DDevice, pImmediateContext );

	//once we have the scene we can initialise the camera GUI components.
	CDXUTComboBox* pCameraSelect = g_SampleUI.GetComboBox( IDC_CAMERASELECT );
	pCameraSelect->RemoveAllItems();
	for( UINT camera = 0; camera < g_Scene.GetNumCameras(); ++camera )
	{
		pCameraSelect->AddItem( g_Scene.GetCameraName( camera ), NULL );
	}
	if( g_CameraIndex >= g_Scene.GetNumCameras() )
	{
		g_CameraIndex = 0;
	}
	pCameraSelect->SetSelectedByIndex( g_CameraIndex );
	g_Scene.SetCamera( g_CameraIndex );

	CreateShaders( pD3DDevice );

    // Create zoombox.
    V_RETURN(g_ZoomBox.OnD3D11CreateDevice(pD3DDevice));

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Create the shaders needed by the app
//--------------------------------------------------------------------------------------
void CreateShaders( ID3D11Device* pD3DDevice )
{
	// compile shaders and input layouts
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
	Utility::CreateVertexShaderAndInputLayoutFromFile( L"PostProcess.hlsl", "VSPostProcess", "vs_4_0",
														pD3DDevice, &g_pPostProcessVertexShader,
														layout, ARRAYSIZE( layout ), &g_pPostProcessInputLayout );

	Utility::CreateVertexShaderAndInputLayoutFromFile( L"PostProcessDynamic.hlsl", "VSPostProcessTemporalAA", "vs_4_0",
														pD3DDevice, &g_pResolveTemporalAAVertexShader,
														layout, ARRAYSIZE( layout ), NULL );

	Utility::CreateVertexShaderAndInputLayoutFromFile( L"PostProcessDynamic.hlsl", "VSPostProcess", "vs_4_0",
														pD3DDevice, &g_pPostProcessDynamicVertexShader,
														layout, ARRAYSIZE( layout ), &g_pPostProcessDynamicInputLayout );

	Utility::CreateVertexShaderAndInputLayoutFromFile( L"PostProcess.hlsl", "VSClear", "vs_4_0",
														pD3DDevice, &g_pClearVertexShader,
														layout, ARRAYSIZE( layout ), NULL );


	Utility::CreatePixelShaderFromFile( L"PostProcess.hlsl","PSMotionBlur", "ps_4_0",
										pD3DDevice, &g_pPostProcessPixelShader );

	Utility::CreatePixelShaderFromFile( L"PostProcess.hlsl","PSClear", "ps_4_0",
										pD3DDevice, &g_pClearPixelShader );

	Utility::CreatePixelShaderFromFile( L"PostProcessDynamic.hlsl","PSMotionBlur", "ps_4_0",
										pD3DDevice, &g_pPostProcessDynamicPixelShader );
	Utility::CreatePixelShaderFromFile( L"PostProcessDynamic.hlsl","PSResolve", "ps_4_0",
										pD3DDevice, &g_pResolveDynamicPixelShader );
	Utility::CreatePixelShaderFromFile( L"PostProcessDynamic.hlsl","PSResolveCubic", "ps_4_0",
										pD3DDevice, &g_pResolveDynamicCubicPixelShader );
	Utility::CreatePixelShaderFromFile( L"PostProcessDynamic.hlsl","PSResolveNoise", "ps_4_0",
										pD3DDevice, &g_pResolveDynamicNoisePixelShader );
	Utility::CreatePixelShaderFromFile( L"PostProcessDynamic.hlsl","PSResolveNoiseOffset", "ps_4_0",
										pD3DDevice, &g_pResolveDynamicNoiseOffsetPixelShader );

	Utility::CreatePixelShaderFromFile( L"PostProcessDynamic.hlsl","PSResolveTemporalAA", "ps_4_0",
										pD3DDevice, &g_pResolveTemporalAAPixelShader );
	Utility::CreatePixelShaderFromFile( L"PostProcessDynamic.hlsl","PSResolveTemporalAAFast", "ps_4_0",
										pD3DDevice, &g_pResolveTemporalAAFastPixelShader );
	Utility::CreatePixelShaderFromFile( L"PostProcessDynamic.hlsl","PSResolveVelTemporalAANoiseOffset", "ps_4_0",
										pD3DDevice, &g_pResolveTemporalAANOPixelShader );
	Utility::CreatePixelShaderFromFile( L"PostProcessDynamic.hlsl","PSResolveVelTemporalAANoiseOffsetFast", "ps_4_0",
										pD3DDevice, &g_pResolveTemporalAANOFastPixelShader );
	Utility::CreatePixelShaderFromFile( L"PostProcessDynamic.hlsl","PSResolveTemporalAABasic", "ps_4_0",
										pD3DDevice, &g_pResolveTemporalAABasicPixelShader );

}

//--------------------------------------------------------------------------------------
// Release the shaders
//--------------------------------------------------------------------------------------
void ReleaseShaders()
{
	SAFE_RELEASE( g_pPostProcessInputLayout );
	SAFE_RELEASE( g_pPostProcessDynamicInputLayout );

    SAFE_RELEASE( g_pPostProcessVertexShader );
    SAFE_RELEASE( g_pPostProcessPixelShader );
    SAFE_RELEASE( g_pPostProcessDynamicVertexShader );
    SAFE_RELEASE( g_pPostProcessDynamicPixelShader );
    SAFE_RELEASE( g_pResolveDynamicPixelShader );
	SAFE_RELEASE( g_pResolveDynamicCubicPixelShader );
	SAFE_RELEASE( g_pResolveDynamicNoisePixelShader );
	SAFE_RELEASE( g_pResolveDynamicNoiseOffsetPixelShader );
	SAFE_RELEASE( g_pClearPixelShader );
	SAFE_RELEASE( g_pClearVertexShader );

	SAFE_RELEASE( g_pResolveTemporalAAVertexShader );
	SAFE_RELEASE( g_pResolveTemporalAAPixelShader );
	SAFE_RELEASE( g_pResolveTemporalAAFastPixelShader );
	SAFE_RELEASE( g_pResolveTemporalAANOPixelShader );
	SAFE_RELEASE( g_pResolveTemporalAANOFastPixelShader );
	SAFE_RELEASE( g_pResolveTemporalAABasicPixelShader );
}

//--------------------------------------------------------------------------------------
// Check for wether device is acceptable.
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo* pAdapterInfo,
                                       UINT uOutput,
                                       const CD3D11EnumDeviceInfo* pDeviceInfo,
                                       DXGI_FORMAT BackBufferFormat,
                                       bool bWindowed,
                                       void* pUserContext )
{
	//force all modes to use stretched scaling, otherwise the default can sometimes switch to centered.
	CD3D11EnumAdapterInfo* pAdapterInfoNonConst = const_cast<CD3D11EnumAdapterInfo*>( pAdapterInfo );
	for( int iDeviceCombo = 0; iDeviceCombo < pAdapterInfoNonConst->deviceSettingsComboList.GetSize(); ++iDeviceCombo )
    {
		CD3D11EnumDeviceSettingsCombo* pDeviceSettingsCombo = pAdapterInfo->deviceSettingsComboList.GetAt( iDeviceCombo );
		//Need to ensure we don't change the scaling for resolutions equal to the output display resolution, so calc max resolution
		UINT maxHeight = 0;
		UINT maxWidth = 0;
		for( int iDisplayMode = 0; iDisplayMode < pDeviceSettingsCombo->pOutputInfo->displayModeList.GetSize(); ++iDisplayMode )
		{
			DXGI_MODE_DESC& mode = pDeviceSettingsCombo->pOutputInfo->displayModeList.GetAt( iDisplayMode );
			//following assumes that the max height and width can occur together at the same time.
			if( maxHeight*maxWidth < mode.Height*mode.Width )
			{
				maxHeight = mode.Height;
				maxWidth = mode.Width;
			}
		}

		for( int iDisplayMode = 0; iDisplayMode < pDeviceSettingsCombo->pOutputInfo->displayModeList.GetSize(); ++iDisplayMode )
		{
			DXGI_MODE_DESC& mode = pDeviceSettingsCombo->pOutputInfo->displayModeList.GetAt( iDisplayMode );
			if( mode.Height != maxHeight || mode.Width != maxWidth )
			{
				//don't stretch if we're at maximum
				mode.Scaling = DXGI_MODE_SCALING_STRETCHED;
			}
		}
	}
 

    return true;
}


//--------------------------------------------------------------------------------------
// Modify the device settings at the start as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings,
                                    void* pUserContext )
{
	//we want 32bit depth buffers without stencil
	pDeviceSettings->d3d11.AutoDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;

	//init defaults
	static bool firstInit = true;
	if( firstInit )
	{
		firstInit = false;
		//we want to have vsycn on as default
		pDeviceSettings->d3d11.SyncInterval = 1;
		//and only be DX10 feature level
		pDeviceSettings->d3d11.DeviceFeatureLevel = D3D_FEATURE_LEVEL_10_0;
	}
    return true;
}


//--------------------------------------------------------------------------------------
// Back buffer dependent initialization 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pD3DDevice,
                                          IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                          void* )
{
    HRESULT hr = S_FALSE;
    V_RETURN( g_DialogResourceManager.OnD3D11ResizedSwapChain( pD3DDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D11ResizedSwapChain( pD3DDevice, pBackBufferSurfaceDesc ) );

    // UI elements placement
    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 200, 30 );
    g_HUD.SetSize( 100, 60 );

    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - g_uGUIWidth, 100 );
    g_SampleUI.SetSize( 100, 40 );

    g_HelpUI.SetLocation( pBackBufferSurfaceDesc->Width - g_uGUIWidth, pBackBufferSurfaceDesc->Height - 50 );
    g_HelpUI.SetSize( 100, 40 );

	//Scene
	g_Scene.OnD3D11ResizedSwapChain( pD3DDevice, pSwapChain, pBackBufferSurfaceDesc );

	//Render Targets
	g_Color.Create( pD3DDevice, DXGI_FORMAT_R8G8B8A8_UNORM, pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height, "Color" );
	g_Velocity.Create( pD3DDevice, DXGI_FORMAT_R16G16_FLOAT, pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height, "Velocity" );



	//viewport for normal rendering
	g_ViewPort.Height = (FLOAT)pBackBufferSurfaceDesc->Height;
	g_ViewPort.Width = (FLOAT)pBackBufferSurfaceDesc->Width;
	g_ViewPort.TopLeftX = 0.0f;
	g_ViewPort.TopLeftY = 0.0f;
	g_ViewPort.MinDepth = 0.0f;
	g_ViewPort.MaxDepth = 1.0f;

	//get refersh rate
	DXGI_SWAP_CHAIN_DESC scDesc;
	hr = pSwapChain->GetDesc( &scDesc );
	if( S_OK == hr  )
	{
		g_VSyncFrameRate =  (float)scDesc.BufferDesc.RefreshRate.Numerator / (float)scDesc.BufferDesc.RefreshRate.Denominator;
	}
	if( scDesc.Windowed )
	{
		//windowed mode refresh rate needs to come from monitor
		DEVMODE displayMode;
		BOOL retval = EnumDisplaySettings(
								NULL,					//__in   LPCTSTR lpszDeviceName, //A NULL value specifies the current display device on the computer on which the calling thread is running.
								ENUM_CURRENT_SETTINGS,	//__in   DWORD iModeNum,
								&displayMode			//__out  DEVMODE *lpDevMode
							);
		if( retval )
		{
			g_VSyncFrameRate = (float)displayMode.dmDisplayFrequency;
		}

	}

	//Resizing window when paused can leave screen blank, so unpause
	g_bPaused = false;
	if( g_SampleUI.GetCheckBox( IDC_PAUSED ) )
	{
			g_SampleUI.GetCheckBox( IDC_PAUSED )->SetChecked( g_bPaused );
	}

	InitDynamicResolution( pD3DDevice );

    V_RETURN(g_ZoomBox.OnD3D11ResizedSwapChain(pD3DDevice, pBackBufferSurfaceDesc));

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Initialise dynamic resolution back buffer dependant objects
//--------------------------------------------------------------------------------------
void InitDynamicResolution( ID3D11Device* pD3DDevice )
{
	//Dynamic Rendering
	g_DynamicResolution.InitializeResolutionParameters( (UINT)g_ViewPort.Width, (UINT)g_ViewPort.Height, g_MaxRTWidth, g_MaxRTHeight, g_ResolutionScaleMax );
	g_DepthBufferDynamic.Create( pD3DDevice, DXGI_FORMAT_D32_FLOAT, g_DynamicResolution.GetDynamicBufferWidth(), g_DynamicResolution.GetDynamicBufferHeight(), "Dynamic Depth" );
	g_ColorDynamic.Create( pD3DDevice, DXGI_FORMAT_R8G8B8A8_UNORM, g_DynamicResolution.GetDynamicBufferWidth(), g_DynamicResolution.GetDynamicBufferHeight(), "Dynamic Color" );
	g_VelocityDynamic[0].Create( pD3DDevice, DXGI_FORMAT_R16G16_FLOAT, g_DynamicResolution.GetDynamicBufferWidth(), g_DynamicResolution.GetDynamicBufferHeight(), "Dynamic Velocity 0" );
	g_VelocityDynamic[1].Create( pD3DDevice, DXGI_FORMAT_R16G16_FLOAT, g_DynamicResolution.GetDynamicBufferWidth(), g_DynamicResolution.GetDynamicBufferHeight(), "Dynamic Velocity 1" );
	g_FinalRTDynamic[0].Create( pD3DDevice, DXGI_FORMAT_R8G8B8A8_UNORM, g_DynamicResolution.GetDynamicBufferWidth(), g_DynamicResolution.GetDynamicBufferHeight(), "Final Color 0" );
	g_FinalRTDynamic[1].Create( pD3DDevice, DXGI_FORMAT_R8G8B8A8_UNORM, g_DynamicResolution.GetDynamicBufferWidth(), g_DynamicResolution.GetDynamicBufferHeight(), "Final Color 1" );

	g_TemporalAAVSConstants.g_VSRTCurrRatio0.x = g_DynamicResolution.GetRTScaleX();
	g_TemporalAAVSConstants.g_VSRTCurrRatio0.y = g_DynamicResolution.GetRTScaleY();
	g_TemporalAAVSConstants.g_VSRTCurrRatio1 = g_TemporalAAVSConstants.g_VSRTCurrRatio0;
	g_TemporalAAVSConstants.g_VSOffset0 = D3DXVECTOR2( 0.0f, 0.0f );
	g_TemporalAAVSConstants.g_VSOffset1 = D3DXVECTOR2( 0.0f, 0.0f );

}

//--------------------------------------------------------------------------------------
// Update the scene
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime,
                           float fElapsedTime,
                           void* pUserContext )
{
	if(  g_bDynamicResolutionEnabled &&
		( RESOLVE_MODE_TEMPORALAA		== g_ResolveMode )  || 
		( RESOLVE_MODE_TEMPORALAA_NO	== g_ResolveMode )  ||
		( RESOLVE_MODE_TEMPORALAA_BASIC	== g_ResolveMode )  )
	{
		g_CurrentRT = 1 - g_CurrentRT;	//switch between 0 and 1
	}
	else
	{
		g_CurrentRT = 0;
	}

	if( !g_bPaused )
	{
		g_SmoothedTimer.Update( fElapsedTime );
		g_Scene.OnFrameMove( g_SmoothedTimer.GetTime(), g_SmoothedTimer.GetElapsedTime(), g_CurrentRT );
	}
	else
	{
		g_Scene.OnFramePaused(  g_CurrentRT );
	}
}

//--------------------------------------------------------------------------------------
// Render the scene
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pD3DDevice,
                                  ID3D11DeviceContext* pD3DImmediateContext,
                                  double fTime,
                                  float fElapsedTime,
                                  void* pUserContext )
{
	//--------------------------------------------------------------------------------------
	// OnD3D11FrameRender Section: setup rendering parameters

    // Check to see if the settings dialog is active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

	float gpuFrameInnerWorkTime, gpuFrameClearTime, gpuFrameSceneTime, gpuFramePostProcTime, gpuFrameScaleTime;
	g_GPUFrameInnerWorkTimer.HaveUpdatedAveragedIntervalTime( pD3DImmediateContext, &gpuFrameInnerWorkTime );
	g_GPUFrameClearTimer.HaveUpdatedAveragedIntervalTime( pD3DImmediateContext, &gpuFrameClearTime );
	// only check if one of the timers is updated, update the GUI from this
	bool bUpdateStats = g_GPUFrameSceneTimer.HaveUpdatedAveragedIntervalTime( pD3DImmediateContext, &gpuFrameSceneTime );
	g_GPUFramePostProcTimer.HaveUpdatedAveragedIntervalTime( pD3DImmediateContext, &gpuFramePostProcTime );
	g_GPUFrameScaleTimer.HaveUpdatedAveragedIntervalTime( pD3DImmediateContext, &gpuFrameScaleTime );


	// control resolution if dynamic resolution enabled
	if( g_bDynamicResolutionEnabled )
	{
		ControlResolution( gpuFrameInnerWorkTime );
	}

	if( bUpdateStats )
	{
		UpdateStats( gpuFrameInnerWorkTime, gpuFrameClearTime, gpuFrameSceneTime, gpuFramePostProcTime, gpuFrameScaleTime );
	}

	//may need to reset dynamic resolution buffers (to handle super sampling)
	if( g_bResetDynamicRes )
	{
		ReleaseDynamicResolution();
		InitDynamicResolution( pD3DDevice );
		g_bResetDynamicRes = false;
	}

	// Temporal Anti-aliasing requires jitter
	D3DXVECTOR2* pJitter = NULL;
	D3DXVECTOR2 jitter;


	//set up RTs for scene render pass
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
	ID3D11RenderTargetView* rtViews[3];
	rtViews[0] = g_Color.GetRenderTargetView();
	rtViews[1] = g_Velocity.GetRenderTargetView();

	//also set up RTs and shader resource views for post process
	ID3D11ShaderResourceView* srViewsPostProcess[5];
	srViewsPostProcess[0] = g_Color.GetShaderResourceView();
	srViewsPostProcess[1] = g_Velocity.GetShaderResourceView();
	srViewsPostProcess[2] = NULL;	// may require 3rd SRV
	srViewsPostProcess[3] = NULL;	// may require 4th SRV
	srViewsPostProcess[4] = NULL;	// may require 5th SRV
	ID3D11RenderTargetView* rtvPostProcess[3];
	rtvPostProcess[0] = DXUTGetD3D11RenderTargetView();
	rtvPostProcess[1] = NULL;	// may require 2nd RT
	D3D11_VIEWPORT	viewPortSceneAndPostProcess( g_ViewPort );

	if( g_bDynamicResolutionEnabled )
	{
		g_DynamicResolution.GetViewport( &viewPortSceneAndPostProcess );

		// Temporal Anti-aliasing jitter calculation
		// TAA switches between two sets of render targets, with odd targets
		// offset by half a pixel. When recombined with an appropriate filter
		// this results in an increase in resolution or anti-aliasing
		if( ( RESOLVE_MODE_TEMPORALAA		== g_ResolveMode )  || 
			( RESOLVE_MODE_TEMPORALAA_NO	== g_ResolveMode )  ||
			( RESOLVE_MODE_TEMPORALAA_BASIC	== g_ResolveMode )  )
		{
	

			//swap 0 to 1
			g_TemporalAAVSConstants.g_VSOffset1 = g_TemporalAAVSConstants.g_VSOffset0;
			g_TemporalAAVSConstants.g_VSRTCurrRatio1 = g_TemporalAAVSConstants.g_VSRTCurrRatio0;

			//init jitter
			D3DXVECTOR2 jitterVectors[2][2] = {
				{
					//asymmetric TAA jitter
					D3DXVECTOR2( 0.0f, 0.0f ),
					D3DXVECTOR2( 0.5f / viewPortSceneAndPostProcess.Width , 0.5f / viewPortSceneAndPostProcess.Height )
				},
				{
					//symmetric TAA jitter
					D3DXVECTOR2( -0.25f / viewPortSceneAndPostProcess.Width , -0.25f / viewPortSceneAndPostProcess.Height ),
					D3DXVECTOR2(  0.25f / viewPortSceneAndPostProcess.Width ,  0.25f / viewPortSceneAndPostProcess.Height )
				} };

			jitter = jitterVectors[ g_SymmetricTAA ][ g_CurrentRT ];

			//setup current constants
			g_TemporalAAVSConstants.g_VSOffset0 = jitter;
			g_TemporalAAVSConstants.g_VSRTCurrRatio0.x = g_DynamicResolution.GetRTScaleX();
			g_TemporalAAVSConstants.g_VSRTCurrRatio0.y = g_DynamicResolution.GetRTScaleY();
			pJitter = &jitter;
		}

		// set up dynamic render targets (note TAA code above could have changed the g_CurrentRT index used)
		rtvPostProcess[0]  = g_FinalRTDynamic[g_CurrentRT].GetRenderTargetView();
		pDSV = g_DepthBufferDynamic.GetDepthStencilView();
		rtViews[0] = g_ColorDynamic.GetRenderTargetView();
		rtViews[1] = g_VelocityDynamic[g_CurrentRT].GetRenderTargetView();
		srViewsPostProcess[0] = g_ColorDynamic.GetShaderResourceView();
		srViewsPostProcess[1] = g_VelocityDynamic[g_CurrentRT].GetShaderResourceView();
	}

	if(!g_bMotionBlur)
	{
		//initial render draws straight to post process RT
		rtViews[0] = rtvPostProcess[0];
	}


	//--------------------------------------------------------------------------------------
	// OnD3D11FrameRender Section: start of scene render code

	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Inner Frame" );
	g_GPUFrameInnerWorkTimer.Begin( pD3DImmediateContext );

	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Clear" );
	g_GPUFrameClearTimer.Begin( pD3DImmediateContext );;


	pD3DImmediateContext->RSSetViewports( 1, &viewPortSceneAndPostProcess );
	pD3DImmediateContext->OMSetRenderTargets( 2, rtViews, pDSV );


	//--------------------------------------------------------------------------------------
	// OnD3D11FrameRender Section: clear section - either standard clear or
	// pixel shader clear. On some platforms pixel shader clear can be faster
	// when the dynamic viewport is smaller than the render target.

	// Clear depth stencil buffers (always do this with standard clear as depth may be hierarchial)
    pD3DImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );


	//clear additional RTs
	if( !g_bClearWithPixelShader )
	{
		static const float ClearToZero[4] =	{ 0.0f, 0.0f, 0.0f, 0.0f };
		pD3DImmediateContext->ClearRenderTargetView( rtViews[1], ClearToZero );
		static const float ClearColor[4] =	{ 0.0f, 0.1f, 0.1f, 1.0f };
		pD3DImmediateContext->ClearRenderTargetView( rtViews[0], ClearColor );
	}
	else
	{
		//clear with pixel shader.
		static const UINT stride = sizeof( D3DXVECTOR3 );
		static const UINT offset = 0;
		pD3DImmediateContext->IASetVertexBuffers( 0, 1, &g_pFullScreenQuadBuffer, &stride, &offset );
		pD3DImmediateContext->IASetIndexBuffer( NULL, DXGI_FORMAT_R16_UINT, 0 );
		pD3DImmediateContext->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
		pD3DImmediateContext->IASetInputLayout( g_pPostProcessInputLayout );
		pD3DImmediateContext->VSSetShader( g_pClearVertexShader, NULL, 0 );
		pD3DImmediateContext->PSSetShader( g_pClearPixelShader, NULL, 0 );
		pD3DImmediateContext->OMSetDepthStencilState( g_pClearDepthStencilState, 0 );
		pD3DImmediateContext->Draw( 4, 0 );
		pD3DImmediateContext->OMSetDepthStencilState( NULL, 0 );	//reset state to default
	}

	g_GPUFrameClearTimer.End( pD3DImmediateContext );
	DXUT_Dynamic_D3DPERF_EndEvent();

	//--------------------------------------------------------------------------------------
	// OnD3D11FrameRender Section: render scene

	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Scene Forwards Render" );
	g_GPUFrameSceneTimer.Begin( pD3DImmediateContext );

	g_Scene.RenderScene( pD3DDevice, pD3DImmediateContext, pJitter );

	g_GPUFrameSceneTimer.End( pD3DImmediateContext );
	DXUT_Dynamic_D3DPERF_EndEvent();


	//--------------------------------------------------------------------------------------
	// OnD3D11FrameRender Section: render post process

	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Post Process Motion Blur" );
	g_GPUFramePostProcTimer.Begin( pD3DImmediateContext );

   // Set render resources
	pD3DImmediateContext->OMSetRenderTargets( 2, rtvPostProcess, NULL );
	pD3DImmediateContext->RSSetViewports( 1, &viewPortSceneAndPostProcess );
	pD3DImmediateContext->PSSetShaderResources( 0, 2, srViewsPostProcess );
	static const UINT stride = sizeof( D3DXVECTOR3 );
	static const UINT offset = 0;
	pD3DImmediateContext->IASetVertexBuffers( 0, 1, &g_pFullScreenQuadBuffer, &stride, &offset );
	pD3DImmediateContext->IASetIndexBuffer( NULL, DXGI_FORMAT_R16_UINT, 0 );
	pD3DImmediateContext->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

 	if( !g_bDynamicResolutionEnabled )
	{
		pD3DImmediateContext->IASetInputLayout( g_pPostProcessInputLayout );
		pD3DImmediateContext->VSSetShader( g_pPostProcessVertexShader, NULL, 0 );
		pD3DImmediateContext->PSSetShader( g_pPostProcessPixelShader, NULL, 0 );
	}
	else
	{

		pD3DImmediateContext->IASetInputLayout( g_pPostProcessDynamicInputLayout );
		pD3DImmediateContext->VSSetShader( g_pPostProcessDynamicVertexShader, NULL, 0 );
		pD3DImmediateContext->PSSetShader( g_pPostProcessDynamicPixelShader, NULL, 0 );

		// Set Constant Buffers
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		pD3DImmediateContext->Map( m_pCBPostProcess, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
		CB_VS_POSTPROCESS* pPostProcess = ( CB_VS_POSTPROCESS* )MappedResource.pData;
		pPostProcess->g_SubSampleRTCurrRatio.x = g_DynamicResolution.GetRTScaleX();
		pPostProcess->g_SubSampleRTCurrRatio.y = g_DynamicResolution.GetRTScaleY();
		pD3DImmediateContext->Unmap( m_pCBPostProcess, 0 );
		pD3DImmediateContext->VSSetConstantBuffers( 0, 1, &m_pCBPostProcess );

		pD3DImmediateContext->Map( m_pCBMotionBlur, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
		CB_PS_MOTIONBLUR* pMotionBlur = ( CB_PS_MOTIONBLUR* )MappedResource.pData;
		pMotionBlur->g_SubSampleRTCurrRatio.x = g_DynamicResolution.GetRTScaleX();
		pMotionBlur->g_SubSampleRTCurrRatio.y = g_DynamicResolution.GetRTScaleY();
		pMotionBlur->g_SubSampleRTCurrRatio.z = g_DynamicResolution.GetDynamicBufferWidth() / g_VelocityToAlphaScale;
		pMotionBlur->g_SubSampleRTCurrRatio.w = g_DynamicResolution.GetDynamicBufferHeight() / g_VelocityToAlphaScale;
		pD3DImmediateContext->Unmap( m_pCBMotionBlur, 0 );
		pD3DImmediateContext->PSSetConstantBuffers( 0, 1, &m_pCBMotionBlur );

		if( RESOLVE_MODE_NONE == g_ResolveMode )
		{
			//clear RT
			static const float ClearToZero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			pD3DImmediateContext->ClearRenderTargetView( rtvPostProcess[0], ClearToZero );
		}
		
	}

	ID3D11SamplerState* samplers[2];
	samplers[0] = g_pSamPoint;
	samplers[1] = g_pSamLinear;
    pD3DImmediateContext->PSSetSamplers( 0, 2, samplers );

	//Render post process
	if(g_bMotionBlur)
	{
		pD3DImmediateContext->Draw( 4, 0 );
	}

	g_GPUFramePostProcTimer.End( pD3DImmediateContext );
	DXUT_Dynamic_D3DPERF_EndEvent();

	//--------------------------------------------------------------------------------------
	// OnD3D11FrameRender Section: render frame scaling resolve to final back buffer
	// (if required)

	DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"Frame Scale" );
	g_GPUFrameScaleTimer.Begin( pD3DImmediateContext );

	if( g_bDynamicResolutionEnabled )
	{
		//render resolve
		if( g_bPaused  && g_CurrentRT )
			srViewsPostProcess[0] = g_FinalRTDynamic[1-g_CurrentRT].GetShaderResourceView();
		else
			srViewsPostProcess[0] = g_FinalRTDynamic[g_CurrentRT].GetShaderResourceView();

		srViewsPostProcess[1] = NULL;	// may require 2nd SRV
		srViewsPostProcess[2] = NULL;	// may require 3rd SRV
		srViewsPostProcess[3] = NULL;	// may require 4th SRV
		srViewsPostProcess[4] = NULL;	// may require 5th SRV

		// Set Constant Buffers
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		pD3DImmediateContext->Map( m_pCBPostProcess, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
		CB_VS_POSTPROCESS* pPostProcess = ( CB_VS_POSTPROCESS* )MappedResource.pData;
		pPostProcess->g_SubSampleRTCurrRatio.x = g_DynamicResolution.GetRTScaleX();
		pPostProcess->g_SubSampleRTCurrRatio.y = g_DynamicResolution.GetRTScaleY();
		pD3DImmediateContext->Unmap( m_pCBPostProcess, 0 );
		pD3DImmediateContext->VSSetConstantBuffers( 0, 1, &m_pCBPostProcess );

		switch( g_ResolveMode )
		{
		case RESOLVE_MODE_NONE:
			{
				//reset scale to 1.0f for upscale
				D3D11_MAPPED_SUBRESOURCE MappedResource;
				pD3DImmediateContext->Map( m_pCBPostProcess, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
				CB_VS_POSTPROCESS* pPostProcess = ( CB_VS_POSTPROCESS* )MappedResource.pData;
				pPostProcess->g_SubSampleRTCurrRatio.x = 1.0f;
				pPostProcess->g_SubSampleRTCurrRatio.y = 1.0f;
				pD3DImmediateContext->Unmap( m_pCBPostProcess, 0 );
				pD3DImmediateContext->VSSetConstantBuffers( 0, 1, &m_pCBPostProcess );

			}
				//carry on to Point mode for samplers and shader
		case RESOLVE_MODE_POINT_MAG:
			pD3DImmediateContext->PSSetSamplers( 0, 1, &g_pSamPoint );
			pD3DImmediateContext->PSSetShader( g_pResolveDynamicPixelShader, NULL, 0 );
			break;
		case RESOLVE_MODE_BILINEAR: 
			pD3DImmediateContext->PSSetSamplers( 0, 1, &g_pSamLinear );
			pD3DImmediateContext->PSSetShader( g_pResolveDynamicPixelShader, NULL, 0 );
			break;
		case RESOLVE_MODE_BICUBIC:
			{
				pD3DImmediateContext->PSSetShader( g_pResolveDynamicCubicPixelShader, NULL, 0 );
				samplers[0] = g_pSamLinear;
				samplers[1] = g_pSamLinearWrap;
				pD3DImmediateContext->PSSetSamplers( 0, 2, samplers );
				// Set Constant Buffer
				D3D11_MAPPED_SUBRESOURCE MappedResource;
				pD3DImmediateContext->Map( m_pCBPSResolveCubic, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
				CB_PS_RESOLVECUBIC* pResolve = ( CB_PS_RESOLVECUBIC* )MappedResource.pData;
				pResolve->g_SizeSource.x = (float)g_DynamicResolution.GetDynamicBufferWidth();
				pResolve->g_SizeSource.y = (float)g_DynamicResolution.GetDynamicBufferHeight();
				pResolve->g_texsize_x.x = 1.0f/pResolve->g_SizeSource.x;
				pResolve->g_texsize_x.y = 0.0f;
				pResolve->g_texsize_y.x = 0.0f;
				pResolve->g_texsize_y.y = 1.0f/pResolve->g_SizeSource.y;
				pD3DImmediateContext->Unmap( m_pCBPSResolveCubic, 0 );
				pD3DImmediateContext->PSSetConstantBuffers( 0, 1, &m_pCBPSResolveCubic );
				srViewsPostProcess[1] = g_pCubicLookupFilterSRV;
			}
			break;
		case RESOLVE_MODE_NOISE:
			{
				pD3DImmediateContext->PSSetShader( g_pResolveDynamicNoisePixelShader, NULL, 0 );
				samplers[0] = g_pSamPoint;
				samplers[1] = g_pSamLinearWrap;
				pD3DImmediateContext->PSSetSamplers( 0, 2, samplers );
				// Set Constant Buffer
				D3D11_MAPPED_SUBRESOURCE MappedResource;
				pD3DImmediateContext->Map( m_pCBPSResolveNoise, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
				CB_PS_RESOLVENOISE* pResolve = ( CB_PS_RESOLVENOISE* )MappedResource.pData;
				// constant scale gives noise pixels scaling with 1/dynamic_resolution
				pResolve->g_NoiseTexScale.x = g_NoiseTextureSize;
				pResolve->g_NoiseTexScale.y = g_NoiseTextureSize;
				pResolve->g_NoiseOffset.x = rand() / ( RAND_MAX + 1.0f );
				pResolve->g_NoiseOffset.y = rand() / ( RAND_MAX + 1.0f );
				pResolve->g_NoiseScale.x = 2.0f - ( g_DynamicResolution.GetRTScaleX() + g_DynamicResolution.GetRTScaleY() );
				pD3DImmediateContext->Unmap( m_pCBPSResolveNoise, 0 );
				pD3DImmediateContext->PSSetConstantBuffers( 0, 1, &m_pCBPSResolveNoise );
				srViewsPostProcess[1] = g_pNoiseTextureSRV;
			}
			break;
		case RESOLVE_MODE_NOISEOFFSET:
			{
				pD3DImmediateContext->PSSetShader( g_pResolveDynamicNoiseOffsetPixelShader, NULL, 0 );
				samplers[0] = g_pSamPoint;
				samplers[1] = g_pSamLinearWrap;
				pD3DImmediateContext->PSSetSamplers( 0, 2, samplers );
				// Set Constant Buffer
				D3D11_MAPPED_SUBRESOURCE MappedResource;
				pD3DImmediateContext->Map( m_pCBPSResolveNoise, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
				CB_PS_RESOLVENOISE* pResolve = ( CB_PS_RESOLVENOISE* )MappedResource.pData;
				pResolve->g_NoiseTexScale.x = g_NoiseTextureSize;
				pResolve->g_NoiseTexScale.y = g_NoiseTextureSize;
				pResolve->g_NoiseScale.x = 0.5f / (float)g_DynamicResolution.GetDynamicBufferWidth();
				pResolve->g_NoiseScale.y = 0.5f / (float)g_DynamicResolution.GetDynamicBufferHeight();
				pD3DImmediateContext->Unmap( m_pCBPSResolveNoise, 0 );
				pD3DImmediateContext->PSSetConstantBuffers( 0, 1, &m_pCBPSResolveNoise );
				srViewsPostProcess[1] = g_pNoiseTextureSRV;
			}
			break;
		case RESOLVE_MODE_TEMPORALAA:
		case RESOLVE_MODE_TEMPORALAA_NO:
		case RESOLVE_MODE_TEMPORALAA_BASIC:
			{
			// Set Vertex Shader and Constant Buffer for Temporal AA
			unsigned int currRT = g_CurrentRT;
			CB_VS_POSTPROCESS_TEMPORAL_AA temporalAAVSConstants = g_TemporalAAVSConstants;
	

			
			if( g_bPaused  && g_CurrentRT )
			{
				// when paused switching the jittered buffer causes flickering due to
				// the previous buffer being faded out by velocity. To resolve this
				// we keep buffer 0 as non jittered, buffer 1 as jittered by switching here
				currRT = 1-g_CurrentRT;
				temporalAAVSConstants.g_VSOffset0		= g_TemporalAAVSConstants.g_VSOffset1;
				temporalAAVSConstants.g_VSOffset1		= g_TemporalAAVSConstants.g_VSOffset0;
				temporalAAVSConstants.g_VSRTCurrRatio0	= g_TemporalAAVSConstants.g_VSRTCurrRatio1;
				temporalAAVSConstants.g_VSRTCurrRatio1	= g_TemporalAAVSConstants.g_VSRTCurrRatio0;
			}
			pD3DImmediateContext->VSSetShader( g_pResolveTemporalAAVertexShader, NULL, 0 );
			D3D11_MAPPED_SUBRESOURCE MappedResource;
			//Map VS constants
			pD3DImmediateContext->Map( m_pCBVSPostProcessTemporalAA, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
			CB_VS_POSTPROCESS_TEMPORAL_AA* pTemporalAAVSConstant = ( CB_VS_POSTPROCESS_TEMPORAL_AA* )MappedResource.pData;
			*pTemporalAAVSConstant = temporalAAVSConstants;
			pD3DImmediateContext->Unmap( m_pCBVSPostProcessTemporalAA, 0 );
			pD3DImmediateContext->VSSetConstantBuffers( 0, 1, &m_pCBVSPostProcessTemporalAA );

			//Map PS constants
			pD3DImmediateContext->Map( m_pCBPSPostProcessTemporalAA, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
			CB_PS_POSTPROCESS_TEMPORAL_AA* pTemporalAAPSConstant = ( CB_PS_POSTPROCESS_TEMPORAL_AA* )MappedResource.pData;

			//choose for scale factor to reduce effect of previous pixel by 1/2 when velocity shows colour data comes from 1 pixel away
			//this requires a scale factor of 1 after we scale the velocity to be in units of pixels
			pTemporalAAPSConstant->g_VelocityScale.x = static_cast<float>( g_DynamicResolution.GetDynamicBufferWidth() );
			pTemporalAAPSConstant->g_VelocityScale.y = static_cast<float>( g_DynamicResolution.GetDynamicBufferHeight() );
			pTemporalAAPSConstant->g_VelocityAlphaScale.x = g_VelocityToAlphaScale * g_VelocityToAlphaScale;
			pD3DImmediateContext->Unmap( m_pCBPSPostProcessTemporalAA, 0 );
			pD3DImmediateContext->PSSetConstantBuffers( 1, 1, &m_pCBPSPostProcessTemporalAA );	//use buffer slot b1


			// Set up srvs.
			srViewsPostProcess[0] = g_FinalRTDynamic[currRT].GetShaderResourceView();
			srViewsPostProcess[1] = g_FinalRTDynamic[1 - currRT].GetShaderResourceView();
			srViewsPostProcess[2] = g_VelocityDynamic[currRT].GetShaderResourceView();
			srViewsPostProcess[3] = g_VelocityDynamic[1 - currRT].GetShaderResourceView();

			if( RESOLVE_MODE_TEMPORALAA == g_ResolveMode )
			{
				pD3DImmediateContext->PSSetSamplers( 0, 1, &g_pSamPoint );
				if( g_bMotionBlur )
				{
					pD3DImmediateContext->PSSetShader( g_pResolveTemporalAAFastPixelShader, NULL, 0 );
				}
				else
				{
					//can't use fast version of shader as alpha not written out in mb shader
					pD3DImmediateContext->PSSetShader( g_pResolveTemporalAAPixelShader, NULL, 0 );
				}
			}
			else if( RESOLVE_MODE_TEMPORALAA_BASIC == g_ResolveMode )
			{
				pD3DImmediateContext->PSSetSamplers( 0, 1, &g_pSamPoint );
				pD3DImmediateContext->PSSetShader( g_pResolveTemporalAABasicPixelShader, NULL, 0 );
			}
			else
			{
				samplers[0] = g_pSamPoint;
				samplers[1] = g_pSamLinearWrap;
				pD3DImmediateContext->PSSetSamplers( 0, 2, samplers );
				if( g_bMotionBlur )
				{
					pD3DImmediateContext->PSSetShader( g_pResolveTemporalAANOFastPixelShader, NULL, 0 );
				}
				else
				{
					//can't use fast version of shader as alpha not written out in mb shader
					pD3DImmediateContext->PSSetShader( g_pResolveTemporalAANOPixelShader, NULL, 0 );
				}
				D3D11_MAPPED_SUBRESOURCE MappedResource;
				pD3DImmediateContext->Map( m_pCBPSResolveNoise, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
				CB_PS_RESOLVENOISE* pResolve = ( CB_PS_RESOLVENOISE* )MappedResource.pData;
				// constant scale gives noise pixels scaling with 1/dynamic_resolution
				pResolve->g_NoiseTexScale.x = g_NoiseTextureSize;
				pResolve->g_NoiseTexScale.y = g_NoiseTextureSize;
				pResolve->g_NoiseScale.x = 0.5f / (float)g_DynamicResolution.GetDynamicBufferWidth();
				pResolve->g_NoiseScale.y = 0.5f / (float)g_DynamicResolution.GetDynamicBufferHeight();
				pD3DImmediateContext->Unmap( m_pCBPSResolveNoise, 0 );
				pD3DImmediateContext->PSSetConstantBuffers( 0, 1, &m_pCBPSResolveNoise );
				srViewsPostProcess[4] = g_pNoiseTextureSRV;
			}

			}
			break;
		}

		rtvPostProcess[0] = DXUTGetD3D11RenderTargetView();
		pD3DImmediateContext->OMSetRenderTargets( 2, rtvPostProcess, NULL );
		pD3DImmediateContext->PSSetShaderResources( 0, 5, srViewsPostProcess );
		pD3DImmediateContext->RSSetViewports( 1, &g_ViewPort );
		pD3DImmediateContext->Draw( 4, 0 );
	}

	//ensure resources no longer bound
	memset( srViewsPostProcess, 0, sizeof( srViewsPostProcess ) );
	pD3DImmediateContext->PSSetShaderResources( 0, sizeof( srViewsPostProcess ) / sizeof( ID3D11ShaderResourceView* ), srViewsPostProcess );
	g_GPUFrameScaleTimer.End( pD3DImmediateContext );
	DXUT_Dynamic_D3DPERF_EndEvent();

    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11Resource*         pRT = NULL;
	pRTV->GetResource(&pRT);

    if(g_ShowZoomBox)
    {
        g_ZoomBox.OnD3D11FrameRender(pD3DDevice, pD3DImmediateContext, NULL, pRT);
    }
    pRT->Release();

    // Display GUI if enabled 
    if( g_bDisplayGUI )
    {
		g_HUD.OnRender( fElapsedTime );
		g_SampleUI.OnRender( fElapsedTime );
		g_HelpUI.OnRender( fElapsedTime );
	}

    // Display the demo text if enabled
    if( g_bDisplayDemoText )
    {
        RenderText();
    }

	g_GPUFrameInnerWorkTimer.End( pD3DImmediateContext );

}

//--------------------------------------------------------------------------------------
// Resolution Control Code
//
// The control method here is a simple proportional method, as we are dealing with a 
// continuously varying quantity which is directly related to the measured input (frame time)
// and which has no momentum.
//
// No hysteris is used since it's a continuous quantity, but the algorithm could be easily
// improved to reduce or even remove resolution changes when the camera is stationary as
// this causes the a slight visible swimming effect.
//--------------------------------------------------------------------------------------
void ControlResolution( float gpuFrameInnerWorkTime )
{
	float optimalElapsedTime = 1.0f/g_VSyncFrameRate;
	switch( g_ControlMode )
	{
	case CONTROL_MODE_MANUAL:		//manual control
		break;

	case CONTROL_MODE_HALFVSYNC:	//vsync/2, so for Vsync of 60FPS this would be 30FPS
		optimalElapsedTime = 1.0f/ (g_VSyncFrameRate / 2.0f);
		//logic continues to next case...

	case CONTROL_MODE_VSYNC:		//Vsync, so 60FPS if this is the monitor frequency
		{
			// use average of measured inner loop time and FPS
			// to account for both CPU and GPU activities to some extent
			float frameTime = 1.0f/DXUTGetFPS();
			float controlTime =  (gpuFrameInnerWorkTime + frameTime)/2.0f;
			if( frameTime > 2.0f * gpuFrameInnerWorkTime )
			{
				// Some activities can cause frame rate spikes in overall time not
				// due to GPU activity, so we ignore these. For example changing resolution & going fullscreen.
				// additionally we may be completely CPU bound so should not throttle GPU too much
				controlTime = gpuFrameInnerWorkTime;
			}
			float timeRatio = controlTime / optimalElapsedTime;
			const float rateOfChange = 0.01f;
			float scaleRatio =  rateOfChange * (1.0f-timeRatio) + 1.0f;
			g_ControlledScale = scaleRatio * g_ControlledScale;
			if( g_ControlledScale < g_ControlledScaleMin )
			{
				g_ControlledScale = g_ControlledScaleMin;
			}
			if( g_ControlledScale > (float)g_ResolutionScaleMax )
			{
				g_ControlledScale = (float)g_ResolutionScaleMax;
			}
			g_DynamicResolution.SetScale( g_ControlledScale, g_ControlledScale );

			//update individual counters
			g_ResolutionScaleX = (UINT)(100.0f*g_DynamicResolution.GetScaleX());
			g_ResolutionScaleY = (UINT)(100.0f*g_DynamicResolution.GetScaleY());
		}
	}
}


//--------------------------------------------------------------------------------------
// Remove resources created in OnD3D11ResizedSwapChain
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11ReleasingSwapChain();
	g_Scene.OnD3D11ReleasingSwapChain();

	g_Color.SafeReleaseAll();
	g_Velocity.SafeReleaseAll();
    g_ZoomBox.OnD3D11ReleasingSwapChain();

	ReleaseDynamicResolution();
}


//--------------------------------------------------------------------------------------
// Remove dynamic resolution resources created in OnD3D11ResizedSwapChain
//--------------------------------------------------------------------------------------
void ReleaseDynamicResolution()
{
	g_ColorDynamic.SafeReleaseAll();
	g_VelocityDynamic[0].SafeReleaseAll();
	g_VelocityDynamic[1].SafeReleaseAll();

	g_DepthBufferDynamic.SafeReleaseAll();
	g_FinalRTDynamic[0].SafeReleaseAll();
	g_FinalRTDynamic[1].SafeReleaseAll();
}

//--------------------------------------------------------------------------------------
// Destroy resources created in OnD3D11CreateDevice
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    g_D3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();

    SAFE_DELETE( g_pTextHelper );

    SAFE_RELEASE( g_pFullScreenQuadBuffer );

	ReleaseShaders();

    SAFE_RELEASE( g_pClearDepthStencilState );
    SAFE_RELEASE( g_pSamPoint );
	SAFE_RELEASE( g_pSamLinear );
	SAFE_RELEASE( g_pSamLinearWrap );
	SAFE_RELEASE( m_pCBPostProcess );
	SAFE_RELEASE( m_pCBMotionBlur );
	SAFE_RELEASE( m_pCBPSResolveCubic );
	SAFE_RELEASE( m_pCBPSResolveNoise );
	SAFE_RELEASE( g_pCubicLookupFilterTex );
	SAFE_RELEASE( g_pCubicLookupFilterSRV );
	SAFE_RELEASE( g_pNoiseTextureSRV );

	SAFE_RELEASE( m_pCBVSPostProcessTemporalAA );
	SAFE_RELEASE( m_pCBPSPostProcessTemporalAA );

	g_GPUFrameInnerWorkTimer.OnD3D11DestroyDevice();
	g_GPUFrameClearTimer.OnD3D11DestroyDevice();
	g_GPUFrameSceneTimer.OnD3D11DestroyDevice();
	g_GPUFramePostProcTimer.OnD3D11DestroyDevice();
	g_GPUFrameScaleTimer.OnD3D11DestroyDevice();
    g_ZoomBox.OnD3D11DestroyDevice();

	g_Scene.OnD3D11DestroyDevice();

}


//--------------------------------------------------------------------------------------
// Application meesage processing
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd,
                          UINT uMsg,
                          WPARAM wParam,
                          LPARAM lParam,
                          bool* pbNoFurtherProcessing,
                          void* puserContext )
{
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
    {
        return 0;
    }

    // If the D3D dialog is active, give it first shot
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // HUD message processing
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
    {
        return 0;
    }

    // SampleUI message processing
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
    {
        return 0;
    }

    // Contact us message processing
    *pbNoFurtherProcessing = g_HelpUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
    {
        return 0;
    }

	//handle scene messages
	g_Scene.HandleMessages( hWnd, uMsg, wParam, lParam );


    return 0;
}

//--------------------------------------------------------------------------------------
// Handle user interaction with GUI 
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT uEvent,
                          int iControlID,
                          CDXUTControl* pControl,
                          void* pUserContext )
{
    switch( iControlID )
    {
        // Handles Toggling Full Screen
    case IDC_TOGGLEFULLSCREEN:
        DXUTToggleFullScreen();
        break;

    case IDC_CHANGEDEVICE:
        g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() );
        break;

    case IDC_RESOLUTIONSCALE_SLIDERX:
        {
            WCHAR sz[100];
            // Get the resolution scale
            g_ResolutionScaleX = ( g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERX )->GetValue() );
            // Update SampleUI text
            swprintf_s( sz, L"Resolution Scale X %%: %d", g_ResolutionScaleX );
            g_SampleUI.GetStatic( IDC_RESOLUTIONSCALE_STATICX )->SetText( sz );
			if( g_bAspectRatioLock )
			{
				g_ResolutionScaleY = g_ResolutionScaleX;
				swprintf_s( sz, L"Resolution Scale Y %%: %d", g_ResolutionScaleY );
				g_SampleUI.GetStatic( IDC_RESOLUTIONSCALE_STATICY )->SetText( sz );
				g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERY )->SetValue( g_ResolutionScaleY );

			}
			g_DynamicResolution.SetScale( g_ResolutionScaleX/100.0f, g_ResolutionScaleY/100.0f );
            break;
        }
    case IDC_RESOLUTIONSCALE_SLIDERY:
        {
            WCHAR sz[100];
            // Get the resolution scale
            g_ResolutionScaleY = ( g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERY )->GetValue() );
            // Update SampleUI text
            swprintf_s( sz, L"Resolution Scale Y %%: %d", g_ResolutionScaleY );
            g_SampleUI.GetStatic( IDC_RESOLUTIONSCALE_STATICY )->SetText( sz );
			if( g_bAspectRatioLock )
			{
				g_ResolutionScaleX = g_ResolutionScaleY;
				swprintf_s( sz, L"Resolution Scale X %%: %d", g_ResolutionScaleX );
				g_SampleUI.GetStatic( IDC_RESOLUTIONSCALE_STATICX )->SetText( sz );
				g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERX )->SetValue( g_ResolutionScaleX );

			}
			g_DynamicResolution.SetScale( g_ResolutionScaleX/100.0f, g_ResolutionScaleY/100.0f );
            break;
        }
	case IDC_DYNAMICRESOLUTION:
		g_bDynamicResolutionEnabled = !g_bDynamicResolutionEnabled;
		g_SampleUI.GetComboBox( IDC_RESOLVEMODE )->SetEnabled( g_bDynamicResolutionEnabled );
		g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERX )->SetEnabled( g_bDynamicResolutionEnabled );
		g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERY )->SetEnabled( g_bDynamicResolutionEnabled );
		g_SampleUI.GetCheckBox( IDC_SUPERSAMPLING )->SetEnabled( g_bDynamicResolutionEnabled );
		g_SampleUI.GetCheckBox( IDC_ASPECTRATIOLOCK )->SetEnabled( g_bDynamicResolutionEnabled );
		g_SampleUI.GetComboBox( IDC_CONTROLMODE )->SetEnabled( g_bDynamicResolutionEnabled );
		break;
    case IDC_RESOLVEMODE:
		g_ResolveMode = (RESOLVE_MODE)g_SampleUI.GetComboBox( IDC_RESOLVEMODE )->GetSelectedIndex();
		if( ( g_ResolveMode == RESOLVE_MODE_TEMPORALAA ) ||
			( g_ResolveMode == RESOLVE_MODE_TEMPORALAA_NO ) ||
			( g_ResolveMode == RESOLVE_MODE_TEMPORALAA_BASIC ) )
		{
			g_SampleUI.GetCheckBox( IDC_SYMMETRIC_TAA )->SetVisible( true );
		}
		else
		{
			g_SampleUI.GetCheckBox( IDC_SYMMETRIC_TAA )->SetVisible( false );
		}
		break;
	case IDC_CAMERASELECT:
		g_CameraIndex = g_SampleUI.GetComboBox( IDC_CAMERASELECT )->GetSelectedIndex();
		g_Scene.SetCamera( g_CameraIndex );
		break;
        // Display the contact message box
    case GUI_CONTACT:
        RenderContactBox();
        break;
    case IDC_TOGGLEZOOM:
        g_ShowZoomBox = !g_ShowZoomBox;
        break;
	case IDC_PAUSED:
		g_bPaused = !g_bPaused;
		break;
	case IDC_MOTIONBLUR:
		g_bMotionBlur = !g_bMotionBlur;
		break;
	case IDC_SYMMETRIC_TAA:
		g_SymmetricTAA = 1 - g_SymmetricTAA; //switch between 0 / 1
		break;
	case IDC_RELOADSHADERS:
		ReleaseShaders();
		CreateShaders( DXUTGetD3D11Device() );
		break;
	case IDC_CLEARWITHPIXELSHADER:
		g_bClearWithPixelShader = !g_bClearWithPixelShader;
		break;
	case IDC_SUPERSAMPLING:
		g_ResolutionScaleMax = 3 - g_ResolutionScaleMax;	// 3 - 1 = 2; 3 - 2 = 1;
		assert( g_ResolutionScaleMax >= 1 );	//not supported
		assert( g_ResolutionScaleMax <= 2 );	//not supported
		g_bResetDynamicRes = true;
		g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERX )->SetRange( 1, g_ResolutionScaleMax*100 );
		g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERX )->SetValue( 1 );	//error in setvalue causes slider to not update unless value passed in does not equal current value, so set to 1 first
		g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERX )->SetValue( g_ResolutionScaleX );
		g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERY )->SetRange( 1, g_ResolutionScaleMax*100 );
		g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERY )->SetValue( 1 );
		g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERY )->SetValue( g_ResolutionScaleY );
		break;
	case IDC_SHOWCAMERAS:
		g_bShowCameras = !g_bShowCameras;
		g_Scene.SetShowCameras( g_bShowCameras );
		break;
	case IDC_ASPECTRATIOLOCK:
		g_bAspectRatioLock = !g_bAspectRatioLock;
		break;
    case IDC_CONTROLMODE:
		g_ControlMode = (CONTROL_MODE)g_SampleUI.GetComboBox( IDC_CONTROLMODE )->GetSelectedIndex();
		// only enable sliders if both dynamic resolution is on and the control mode is manual.
		g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERX )->SetEnabled( g_bDynamicResolutionEnabled && ( g_ControlMode == CONTROL_MODE_MANUAL ) );
		g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERY )->SetEnabled( g_bDynamicResolutionEnabled && ( g_ControlMode == CONTROL_MODE_MANUAL ) );
		break;
    }
}


//--------------------------------------------------------------------------------------
// Key press handling
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT uChar,
                          bool bKeyDown,
                          bool bAltDown,
                          void* pUserContext )
{
    if( bKeyDown )
    {
        switch( uChar )
        {
        case VK_F1:
            g_bShowHelp = !g_bShowHelp;
            break;
        case VK_F10:
            g_bDisplayGUI = !g_bDisplayGUI;
            break;
        case VK_F11:
            g_bDisplayDemoText = !g_bDisplayDemoText;
            break;
		default:
			break;
        }
    }
}


//--------------------------------------------------------------------------------------
// Mouse handling code 
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse( bool bLeftButtonDown,
                       bool bRightButtonDown,
                       bool bMiddleButtonDown,
                       bool bSideButton1Down,
                       bool bSideButton2Down,
                       int nMouseWheelDelta,
                       int xPos,
                       int yPos,
                       void* pUserContext )
{
    if(bRightButtonDown)
    {
        RECT ClientRect = DXUTGetWindowClientRect();
        FLOAT x =  xPos / (FLOAT) ClientRect.right;
        FLOAT y =  yPos / (FLOAT) ClientRect.bottom;
        g_ZoomBox.SetZoomCenterPosition(x, y);
    }
}

//--------------------------------------------------------------------------------------
// Update Stats
//--------------------------------------------------------------------------------------
void UpdateStats( float gpuFrameInnerWorkTime, float gpuFrameClearTime, float gpuFrameSceneTime, float gpuFramePostProcTime, float gpuFrameScaleTime )
{
	// Update text occasionally
	WCHAR sz[100];
	// Update timing information
	swprintf_s( sz, L"Frame Time (ms): %.2f", gpuFrameInnerWorkTime*1000.0f );
	g_SampleUI.GetStatic( IDC_FRAMETIMESTATIC )->SetText( sz );
	swprintf_s( sz, L"Clear Time (ms): %.2f", gpuFrameClearTime*1000.0f );
	g_SampleUI.GetStatic( IDC_CLEARTIMESTATIC )->SetText( sz );
	swprintf_s( sz, L"Scene Time (ms): %.2f", gpuFrameSceneTime*1000.0f );
	g_SampleUI.GetStatic( IDC_SCENETIMESTATIC )->SetText( sz );
	swprintf_s( sz, L"PostP Time (ms): %.2f", gpuFramePostProcTime*1000.0f );
	g_SampleUI.GetStatic( IDC_POSTPROCTIMESTATIC )->SetText( sz );
	swprintf_s( sz, L"Scale Time (ms): %.2f", gpuFrameScaleTime*1000.0f );
	g_SampleUI.GetStatic( IDC_SCALETIMESTATIC )->SetText( sz );
	swprintf_s( sz, L"VSync Time (ms): %.2f", (1.0f/g_VSyncFrameRate)*1000.0f );
	g_SampleUI.GetStatic( IDC_VSYNCFRAMERATESTATIC )->SetText( sz );

	// Update scale text and sliders. We get the scale text always from actual scale,
	// but slide value from the control variables to prevent the internal changes in scale x and y
	// effecting the desired control value
    swprintf_s( sz, L"Resolution Scale X %%: %d", (UINT)(100.0f*g_DynamicResolution.GetScaleX()) );
    g_SampleUI.GetStatic( IDC_RESOLUTIONSCALE_STATICX )->SetText( sz );
	g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERX )->SetValue( g_ResolutionScaleX );
	swprintf_s( sz, L"Resolution Scale Y %%: %d", (UINT)(100.0f*g_DynamicResolution.GetScaleY()) );
	g_SampleUI.GetStatic( IDC_RESOLUTIONSCALE_STATICY )->SetText( sz );
	g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERY )->SetValue( g_ResolutionScaleY );

}


//--------------------------------------------------------------------------------------
// Pre-init phase 
//--------------------------------------------------------------------------------------
void InitApp()
{
    // Dialogs init
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );
    g_HelpUI.Init( &g_DialogResourceManager );

    // HUD buttons and handling callback
    g_HUD.SetCallback( OnGUIEvent );
    int iY = 0;
    g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Fullscreen (F2)", 0, iY, g_uGUIWidth, g_uGUIHeight, VK_F2 );
    g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F3)", 0, iY += 26, g_uGUIWidth, g_uGUIHeight, VK_F3 );

	// Add Shader Reoload Button for runtime recompile of shaders
    g_HUD.AddButton( IDC_RELOADSHADERS, L"Reload Shaders (F4)", 0, iY += 26, g_uGUIWidth, g_uGUIHeight, VK_F4 );

	// Add Sample UI
    g_SampleUI.SetCallback( OnGUIEvent );
    iY = 0;
	// Add Pause
	g_SampleUI.AddCheckBox( IDC_PAUSED, L"Paused (P)", 0, iY += 26, 170, g_uGUIHeight, g_bPaused, 'P' );
	// Add Show Cameras
	g_SampleUI.AddCheckBox( IDC_SHOWCAMERAS, L"Show Cameras", 0, iY += 26, 170, g_uGUIHeight, g_bShowCameras );
	
	// Add Toggle for Motion Blur
	g_SampleUI.AddCheckBox( IDC_MOTIONBLUR, L"MotionBlur", 0, iY += 26, 170, g_uGUIHeight, g_bMotionBlur );

	//Add show zoom box, which allows zoomed inspection of back buffer
	g_SampleUI.AddCheckBox(IDC_TOGGLEZOOM,     L"Show zoom box (Z)",   0, iY += 26, 170, 23, g_ShowZoomBox, 'Z');

	// Add camera selection combo box, which gets filled in later.
    g_SampleUI.AddStatic( IDC_CAMERASELECTSTATIC, L"Camera:", 0, iY += 26, 120, g_uGUIHeight );
	CDXUTComboBox*	pCameraSelect;
    g_SampleUI.AddComboBox( IDC_CAMERASELECT, 0, iY += 18, 170, g_uGUIHeight, 0, false, &pCameraSelect );

	// Add Clear with Pixel Shader
	g_SampleUI.AddCheckBox( IDC_CLEARWITHPIXELSHADER, L"Clear with Pixel Shader", 0, iY += 26, 170, g_uGUIHeight, g_bClearWithPixelShader );


	//Add checkbox for dynamic resolution being enabled
	g_SampleUI.AddCheckBox( IDC_DYNAMICRESOLUTION, L"Dynamic Resolution", 0, iY += 26, 170, g_uGUIHeight, g_bDynamicResolutionEnabled );

	//Add checkbox for supersampling being enabled
	g_SampleUI.AddCheckBox( IDC_SUPERSAMPLING, L"Allow Super Sampling", 0, iY += 26, 170, g_uGUIHeight, g_ResolutionScaleMax != 1 );
	g_SampleUI.GetCheckBox( IDC_SUPERSAMPLING )->SetEnabled( g_bDynamicResolutionEnabled );


	//Add checkbox for supersampling being enabled
	g_SampleUI.AddCheckBox( IDC_ASPECTRATIOLOCK, L"Aspect Ratio Lock", 0, iY += 26, 170, g_uGUIHeight, g_bAspectRatioLock );
	g_SampleUI.GetCheckBox( IDC_ASPECTRATIOLOCK )->SetEnabled( g_bDynamicResolutionEnabled );
	
	// Add resolution scale to SampleUI 
    // X
    g_ResolutionScaleX = 100; //percent
    WCHAR sz[100];
    iY += 24;
    // Set the slider default values
    swprintf_s( sz, L"Resolution Scale X %%: %d", g_ResolutionScaleX );
    g_SampleUI.AddStatic( IDC_RESOLUTIONSCALE_STATICX, sz, 0, iY, 120, g_uGUIHeight );
    g_SampleUI.AddSlider( IDC_RESOLUTIONSCALE_SLIDERX, 0, iY += 18, 120, g_uGUIHeight, 1, g_ResolutionScaleMax*100, g_ResolutionScaleX );
	g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERX )->SetEnabled( g_bDynamicResolutionEnabled );
    // Y
    g_ResolutionScaleY = 100; //percent
    iY += 24;
    // Set the slider default values
    swprintf_s( sz, L"Resolution Scale Y %%: %d", g_ResolutionScaleY );
    g_SampleUI.AddStatic( IDC_RESOLUTIONSCALE_STATICY, sz, 0, iY, 120, g_uGUIHeight );
    g_SampleUI.AddSlider( IDC_RESOLUTIONSCALE_SLIDERY, 0, iY += 18, 120, g_uGUIHeight, 1, g_ResolutionScaleMax*100, g_ResolutionScaleY );
	g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERY )->SetEnabled( g_bDynamicResolutionEnabled );

	// Add filter mode for resolve selection
    g_SampleUI.AddStatic( IDC_RESOLVEMODESTATIC, L"Filter:", 0, iY += 26, 120, g_uGUIHeight );
	CDXUTComboBox*	pResolveSelect;
    g_SampleUI.AddComboBox( IDC_RESOLVEMODE, 0, iY += 18, 170, g_uGUIHeight, 0, false, &pResolveSelect );
	pResolveSelect->AddItem( L"None (Debug)", NULL );
	pResolveSelect->AddItem( L"Point", NULL );
	pResolveSelect->AddItem( L"Bilinear", NULL );
	pResolveSelect->AddItem( L"Bicubic", NULL );
	pResolveSelect->AddItem( L"Noise", NULL );
	pResolveSelect->AddItem( L"Noise Offset", NULL );
	pResolveSelect->AddItem( L"Temporal AA", NULL );
	pResolveSelect->AddItem( L"TAA Noise Off.", NULL );
	pResolveSelect->AddItem( L"TAA Basic", NULL );
	pResolveSelect->SetEnabled( g_bDynamicResolutionEnabled );
	pResolveSelect->SetSelectedByIndex( g_ResolveMode );

	//Add new Symmetric Temporal AA check box. This switches bwteen asymmetric TAA using offsets [0,0],[0.5,0.5],
	// and symmetric TAA with [-0.25,-0.25],[+0.25,+0.25]
	g_SampleUI.AddCheckBox(IDC_SYMMETRIC_TAA,  L"Symmetric TAA",   0, iY += 26, 170, 23, g_SymmetricTAA == 1 );
	if( ( g_ResolveMode == RESOLVE_MODE_TEMPORALAA ) ||
		( g_ResolveMode == RESOLVE_MODE_TEMPORALAA_NO ) ||
		( g_ResolveMode == RESOLVE_MODE_TEMPORALAA_BASIC ) )
	{
		g_SampleUI.GetCheckBox( IDC_SYMMETRIC_TAA )->SetVisible( true );
	}
	else
	{
		g_SampleUI.GetCheckBox( IDC_SYMMETRIC_TAA )->SetVisible( false );
	}

	// Add Control mechanism
    g_SampleUI.AddStatic( IDC_CONTROLMODESTATIC, L"Resolution Control:", 0, iY += 26, 120, g_uGUIHeight );
	CDXUTComboBox*	pControlSelect;
    g_SampleUI.AddComboBox( IDC_CONTROLMODE, 0, iY += 18, 170, g_uGUIHeight, 0, false, &pControlSelect );
	pControlSelect->AddItem( L"Manual", NULL );
	pControlSelect->AddItem( L"VSync/2", NULL );
	pControlSelect->AddItem( L"VSync", NULL );
	pControlSelect->SetEnabled( g_bDynamicResolutionEnabled );
	pControlSelect->SetSelectedByIndex( g_ControlMode );
	g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERX )->SetEnabled( g_bDynamicResolutionEnabled && ( g_ControlMode == CONTROL_MODE_MANUAL ) );
	g_SampleUI.GetSlider( IDC_RESOLUTIONSCALE_SLIDERY )->SetEnabled( g_bDynamicResolutionEnabled && ( g_ControlMode == CONTROL_MODE_MANUAL ) );


	// Add Performance counters
	g_SampleUI.AddStatic( IDC_FRAMETIMESTATIC, L"Frame Time (ms): NA", 0, iY += 26, 120, g_uGUIHeight );
	g_SampleUI.AddStatic( IDC_CLEARTIMESTATIC, L"Clear Time (ms): NA", 0, iY += 12, 120, g_uGUIHeight );
	g_SampleUI.AddStatic( IDC_SCENETIMESTATIC, L"Scene Time (ms): NA", 0, iY += 12, 120, g_uGUIHeight );
	g_SampleUI.AddStatic( IDC_POSTPROCTIMESTATIC, L"PostP Time (ms): NA", 0, iY += 12, 120, g_uGUIHeight );
	g_SampleUI.AddStatic( IDC_SCALETIMESTATIC, L"Scale Time (ms): NA", 0, iY += 12, 120, g_uGUIHeight );
	g_SampleUI.AddStatic( IDC_VSYNCFRAMERATESTATIC, L"Vsync Time (ms): NA", 0, iY += 12, 120, g_uGUIHeight );


    // Contact button and handling callback
    g_HelpUI.SetCallback( OnGUIEvent );
    iY = 0;
    g_HelpUI.AddButton( GUI_CONTACT, L"Contact Us", 0, iY, 100, 30 );

	g_DynamicResolution.SetScale( 1.0f, 1.0f );

}

//--------------------------------------------------------------------------------------
// Render the demo text 
//--------------------------------------------------------------------------------------
void RenderText()
{
    int iX = 2, iY = 0;
    g_pTextHelper->Begin();
    g_pTextHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTextHelper->SetInsertionPos( iX, iY );

    // Draw the device information
    g_pTextHelper->DrawTextLine( DXUTGetFrameStats( true ) );
    g_pTextHelper->SetInsertionPos( iX, iY += 20 );
    g_pTextHelper->DrawTextLine( DXUTGetDeviceStats() );


    if( !g_bShowHelp ) // Help screen disabled
    {
        g_pTextHelper->SetInsertionPos( iX, iY += 20 );
        g_pTextHelper->DrawTextLine( L"Press F1 For Help" );
    }
    else // Help screen enabled 
    {
        // Render the default help screen
        g_pTextHelper->SetInsertionPos( 10, ( int )( g_DialogResourceManager.m_nBackBufferHeight * 0.7 ) );	
 
		g_pTextHelper->DrawTextLine( L"Controls:" );
	
		//Render shortcut keys.
		g_pTextHelper->DrawTextLine( L"Fullscreen: F2" );
		g_pTextHelper->DrawTextLine( L"Change device: F3" );
		g_pTextHelper->DrawTextLine( L"Toggle warp: F4" );
		g_pTextHelper->DrawTextLine( L"Toggle GUI: F10" );
		g_pTextHelper->DrawTextLine( L"Toggle Demo Text: F11" );
		g_pTextHelper->DrawTextLine( L"Camera Controls:" );
		g_pTextHelper->DrawTextLine( L"    To Activate Camera Controls Select First Person Camera:" );
		g_pTextHelper->DrawTextLine( L"    Rotate Camera: Left Mouse Button" );
		g_pTextHelper->DrawTextLine( L"    Movement: WASD Keys" );
		g_pTextHelper->DrawTextLine( L"Hide Help: F1" );								
		g_pTextHelper->DrawTextLine( L"Quit: ESC" );	

    }
    g_pTextHelper->End();
}

//--------------------------------------------------------------------------------------
// Main function
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPWSTR szCmdLine,
                     int iCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Init DXUT
    //DXUTInit( true, true, L"-forcevsync:1" );
	DXUTInit( true, true, NULL );
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"Dynamic Resolution Rendering" );

    // Device callbacks
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );

    // DXUT callbacks
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackMouse( OnMouse );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    // Application level intiailization
    InitApp();

    //Create device and go!
    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 1280, 720 );
    DXUTMainLoop();

    return DXUTGetExitCode();
}

