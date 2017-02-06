// Copyright 2010 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies. Intel makes no representations about the
// suitability of this software for any purpose. THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

cbuffer cbChangesRarely : register( b0 )
{
    float2 g_ZoomPos;
}

//-----------------------------------------------------------------------------
// Textures and Samplers
//-----------------------------------------------------------------------------

Texture2D     g_txZoom         : register( t0 );
SamplerState  g_samZoom        : register( s0 );

//-----------------------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------------------

struct VS_INPUT
{
    float3 vPos             : POSITION;
};

struct GS_DUMMY_INPUT
{
};

struct PS_INPUT_BORDER
{
    float4 vPos             : SV_POSITION;
};

struct PS_INPUT_ZOOM_QUAD
{
    float4 vPos             : SV_POSITION;
	float2 vTCZoom          : TEXCOORD0;
};

//-----------------------------------------------------------------------------
// Vertex Shader
//-----------------------------------------------------------------------------

VS_INPUT VS( VS_INPUT input )
{
    // A simple pass-thru (real geometry generated in GS)
    return input;
}

//-----------------------------------------------------------------------------
// Geometry Shaders
//-----------------------------------------------------------------------------

[MaxVertexCount(12)] 
void GS_Zoom( point GS_DUMMY_INPUT input[1], inout TriangleStream<PS_INPUT_ZOOM_QUAD> TriStream )
{
    PS_INPUT_ZOOM_QUAD Verts[4];

    Verts[0].vPos    = float4(-0.55, -0.2, 0.5, 1.0);
    Verts[0].vTCZoom = float2(g_ZoomPos.x + 0.01, g_ZoomPos.y + 0.01); 

    Verts[1].vPos    = float4(-0.95, -0.2, 0.5, 1.0);
    Verts[1].vTCZoom = float2(g_ZoomPos.x - 0.01, g_ZoomPos.y + 0.01);
        
    Verts[2].vPos    = float4(-0.55, +0.2, 0.5, 1.0);
    Verts[2].vTCZoom = float2(g_ZoomPos.x + 0.01, g_ZoomPos.y - 0.01);
    
    Verts[3].vPos    = float4(-0.95, +0.2, 0.5, 1.0);
    Verts[3].vTCZoom = float2(g_ZoomPos.x -0.01, g_ZoomPos.y - 0.01);

    for(int i = 0; i < 4; ++i)
    {
        TriStream.Append(Verts[i]);
    }

    TriStream.RestartStrip();
}


[MaxVertexCount(12)]
void GS_Border( point GS_DUMMY_INPUT input[1], inout LineStream<PS_INPUT_BORDER> LineStrip )
{
    PS_INPUT_BORDER Verts[12];

    // Large box where the zoomed in texture is displayed
    Verts[0].vPos = float4(-0.54, -0.21, 0.0, 1.0);
    Verts[1].vPos = float4(-0.54, +0.21, 0.0, 1.0);
    Verts[2].vPos = float4(-0.96, +0.21, 0.0, 1.0);
    Verts[3].vPos = float4(-0.96, -0.21, 0.0, 1.0);
    Verts[4].vPos = float4(-0.54, -0.21, 0.0, 1.0);

    float4 vCenterMouse = float4((g_ZoomPos.x - 0.5) * 2.0, (0.5 - g_ZoomPos.y) * 2.0, 0.0, 1.0);

    // Small box centered around the mouse pointer
    Verts[5].vPos  = float4(vCenterMouse.x - 0.02, vCenterMouse.y - 0.02, 0.0, 1.0);
    Verts[6].vPos  = float4(vCenterMouse.x - 0.02, vCenterMouse.y + 0.02, 0.0, 1.0);
    Verts[7].vPos  = float4(vCenterMouse.x + 0.02, vCenterMouse.y + 0.02, 0.0, 1.0);
    Verts[8].vPos  = float4(vCenterMouse.x + 0.02, vCenterMouse.y - 0.02, 0.0, 1.0);
    Verts[9].vPos  = float4(vCenterMouse.x - 0.02, vCenterMouse.y - 0.02, 0.0, 1.0);
    Verts[10].vPos = float4(vCenterMouse.x - 0.02, vCenterMouse.y + 0.02, 0.0, 1.0);

    Verts[11].vPos = float4(-0.54, 0.21, 0.0, 1.0);

    for(int i = 0; i < 12; ++i)
    {
        LineStrip.Append(Verts[i]);
    }
}

//-----------------------------------------------------------------------------
// Pixel Shaders
//-----------------------------------------------------------------------------

float4 PS_Zoom( PS_INPUT_ZOOM_QUAD input ) : SV_Target
{
    return g_txZoom.Sample(g_samZoom, input.vTCZoom);
}


float4 PS_Border( PS_INPUT_BORDER input ) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

