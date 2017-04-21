/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////


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

