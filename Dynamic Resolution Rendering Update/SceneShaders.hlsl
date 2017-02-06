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
//--------------------------------------------------------------------------------------


cbuffer cbPSPerModel : register( b0 )
{
	float4		g_vDiffuseColor		: packoffset(c0);
	float4		g_vAmbientColor		: packoffset(c1);
	float4		g_vSpecularColor	: packoffset(c2);
	float3		g_vLightDir			: packoffset(c3);
	float		g_fSpecularPow		: packoffset(c3.w);
};


// Textures and Samplers
Texture2D		g_txDiffuse			: register( t0 );
Texture2D		g_txNormal			: register( t1 );
Texture2D		g_txSpecular		: register( t2 );


// Sampler state
SamplerState	g_samLinear			: register( s0 );

cbuffer cbVSPerObject : register(b0)
{
	matrix		g_mWorldViewProjection		: packoffset(c0);
	matrix		g_mPrevWorldViewProjection	: packoffset(c4);
	matrix		g_mWorld					: packoffset(c8);
	float4		g_vEyePos					: packoffset(c12);
};


// VS input
struct VS_INPUT
{
	float4 position		: POSITION;
	float3 normal		: NORMAL;
	float2 texcoord		: TEXCOORD0;
	float3 tangent		: TANGENT;
	float3 binormal		: BINORMAL;
};

// PS input
struct PS_INPUT
{
	float4 position		: SV_POSITION;
	float3 normal		: NORMAL;
	float2 texcoord		: TEXCOORD0;
	float3 tangent		: TANGENT;
	float3 binormal		: BINORMAL;
	float3 viewDir		: TEXCOORD1;
	float4 curr_pos		: TEXCOORD2;
	float4 prev_pos		: TEXCOORD3;
};


//-----------------------------------------------------------------------------------------
// Vertex Shader for simple normal and specular mapping with per pixel velocity output
//-----------------------------------------------------------------------------------------
PS_INPUT VSMain( VS_INPUT Input )
{
	PS_INPUT Output;
	
	Output.position = mul(Input.position, g_mWorldViewProjection);
	Output.normal = mul(Input.normal, (float3x3)g_mWorld);
	Output.texcoord = Input.texcoord;
	Output.tangent	= mul(Input.tangent, (float3x3)g_mWorld);
	Output.binormal = mul(Input.binormal, (float3x3)g_mWorld);
		
	float4 worldPos;
	worldPos = mul(Input.position, g_mWorld);

    // Determine the viewing direction based on the position of the camera and the position of the vertex in the world.
    Output.viewDir = g_vEyePos.xyz - worldPos.xyz;

	// calulate screen space velocity - output current and previous position
	// which will interpolate linearly well, allowing pixel shader to calculate
	// velocity
	float4 prevPosition = mul(Input.position, g_mPrevWorldViewProjection);
	Output.curr_pos =  Output.position;
	Output.prev_pos = prevPosition;

	return Output;
}

struct PS_OUTPUT
{
    float4 Color			: SV_TARGET0; 
    float2 Velocity			: SV_TARGET1;
};

//-----------------------------------------------------------------------------------------
// Pixel Shader for simple normal and specular mapping with per pixel velocity output
// Vertex shader sets up position, normals and tangents.
//-----------------------------------------------------------------------------------------
PS_OUTPUT PSMain( PS_INPUT Input )
{
	PS_OUTPUT Output;
	float4 texColor = g_txDiffuse.Sample( g_samLinear, Input.texcoord );
	if( texColor.a == 0.0f )
	{
		discard;
	}
	float4 normal = 2.0f * g_txNormal.Sample( g_samLinear, Input.texcoord ) - 1.0f;

    float3 bump = normalize( normal.z * normalize( Input.normal ) + normal.x * normalize( Input.tangent ) + normal.y * normalize ( Input.binormal )); 
    float3 lightDir = -g_vLightDir;
    float lightIntensity = saturate(dot(bump, lightDir));
    
    float4 color = saturate(g_vDiffuseColor * lightIntensity) + g_vAmbientColor;
	color *= texColor;

    if(lightIntensity > 0.0f)
    {
		float4 specularIntensity = g_txSpecular.Sample( g_samLinear, Input.texcoord );
        float3 reflection = normalize(2 * lightIntensity * bump - lightDir); 
        float4 specular = pow(saturate(dot(reflection, normalize( Input.viewDir ) )), g_fSpecularPow);

        specular = specular * specularIntensity * g_vSpecularColor;
        color = saturate(color + specular);
    }
	
	Output.Color = color;
	Output.Velocity = Input.curr_pos.xy/Input.curr_pos.w - Input.prev_pos.xy/Input.prev_pos.w;
	Output.Velocity.y = -Output.Velocity.y;	// texture coordinates have Y inverse of screen coordinates.
	Output.Velocity /= 2.0f;				// texture is 0 - 1 rather than -1 to +1 so half size.
    return Output;
}



