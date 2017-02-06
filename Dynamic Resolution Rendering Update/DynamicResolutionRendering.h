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

#ifndef	__SAMPLESTART_H_
#define	__SAMPLESTART_H_

#include "DXUT.h"
#include "DXUTmisc.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"

#include "SDKmisc.h"
#include "SDKMesh.h"

// SampleComponents
#include "SampleComponentsNoTBB.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' ""version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Global Variables UI control IDs
#define IDC_TOGGLEFULLSCREEN			1
#define IDC_CHANGEDEVICE				2
#define GUI_CONTACT						8
#define IDC_RESOLUTIONSCALE_SLIDERX		9
#define IDC_RESOLUTIONSCALE_SLIDERY		10
#define IDC_RESOLUTIONSCALE_STATICX		11
#define IDC_RESOLUTIONSCALE_STATICY		12
#define IDC_DYNAMICRESOLUTION			13
#define IDC_RESOLVEMODE					14
#define IDC_RESOLVEMODESTATIC			15
#define IDC_PAUSED						16
#define IDC_CLEARWITHPIXELSHADER		17
#define IDC_SUPERSAMPLING				18
#define IDC_SHOWCAMERAS					19
#define IDC_ASPECTRATIOLOCK				20
#define IDC_CAMERASELECT				21
#define IDC_CAMERASELECTSTATIC			21
#define IDC_FRAMETIMESTATIC				22
#define IDC_CLEARTIMESTATIC				23
#define IDC_SCENETIMESTATIC				24
#define IDC_POSTPROCTIMESTATIC			25
#define IDC_SCALETIMESTATIC				26
#define IDC_VSYNCFRAMERATESTATIC		27
#define IDC_CONTROLMODE					28
#define IDC_CONTROLMODESTATIC			29
#define IDC_MOTIONBLUR					30
#define IDC_RELOADSHADERS				31
#define IDC_TOGGLEZOOM                  32
#define IDC_SYMMETRIC_TAA               33




enum RESOLVE_MODE
{
	RESOLVE_MODE_NONE			= 0,
	RESOLVE_MODE_POINT_MAG		= 1,
	RESOLVE_MODE_BILINEAR		= 2,
	RESOLVE_MODE_BICUBIC		= 3,
	RESOLVE_MODE_NOISE			= 4,
	RESOLVE_MODE_NOISEOFFSET	= 5,
	RESOLVE_MODE_TEMPORALAA		= 6,
	RESOLVE_MODE_TEMPORALAA_NO	= 7,
	RESOLVE_MODE_TEMPORALAA_BASIC = 8
};

enum CONTROL_MODE
{
	CONTROL_MODE_MANUAL			= 0,
	CONTROL_MODE_HALFVSYNC		= 1,
	CONTROL_MODE_VSYNC			= 2
};

// D3D device callback
HRESULT CALLBACK    OnD3D11CreateDevice( ID3D11Device*,
                                         const DXGI_SURFACE_DESC*,
                                         void* );

// Is the device acceptable?
bool CALLBACK       IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo*,
                                             UINT,
                                             const CD3D11EnumDeviceInfo,
                                             DXGI_FORMAT,
                                             bool,
                                             void* );

// Device modify settings
bool CALLBACK       ModifyDeviceSettings( DXUTDeviceSettings*,
                                          void* );

// SC Resized callback
HRESULT CALLBACK    OnD3D11ResizedSwapChain( ID3D11Device*,
                                             IDXGISwapChain*,
                                             const DXGI_SURFACE_DESC*,
                                             void* );

// Frame move callback
void CALLBACK       OnFrameMove( double,
                                 float,
                                 void* );

// Frame render callback
void CALLBACK       OnD3D11FrameRender( ID3D11Device*,
                                        ID3D11DeviceContext*,
                                        double,
                                        float,
                                        void* );


// Swap chain release callback
void CALLBACK       OnD3D11ReleasingSwapChain( void* );

// Destroy device callback
void CALLBACK       OnD3D11DestroyDevice( void* );

// Message processing
LRESULT CALLBACK    MsgProc( HWND,
                             UINT,
                             WPARAM,
                             LPARAM,
                             bool*,
                             void* );

// UI event handling
void CALLBACK       OnGUIEvent( UINT,
                                int,
                                CDXUTControl*,
                                void* );

// Keyboard handling
void CALLBACK       OnKeyboard( UINT,
                                bool,
                                bool,
                                void* );

// Mouse handling
void CALLBACK       OnMouse( bool,
                             bool,
                             bool,
                             bool,
                             bool,
                             int,
                             int,
                             int,
                             void* );

// Pre-init code
void InitApp();

// GUI text drawing
void RenderText();

// Update Stats
void UpdateStats( float gpuFrameInnerWorkTime, float gpuFrameClearTime, float gpuFrameSceneTime, float gpuFramePostProcTime, float gpuFrameScaleTime );

// Resolution Control Code
void ControlResolution( float gpuFrameInnerWorkTime );

// Main function
int WINAPI          wWinMain( HINSTANCE,
                              HINSTANCE,
                              LPWSTR,
                              int );

void ReleaseDynamicResolution();

void InitDynamicResolution( ID3D11Device* pD3DDevice );

void CreateShaders( ID3D11Device* pD3DDevice );
void ReleaseShaders();

#endif //__SAMPLESTART_H_
