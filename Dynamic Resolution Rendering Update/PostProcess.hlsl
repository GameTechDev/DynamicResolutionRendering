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

struct VSPostProcessIn
{
	float4 Pos	: POSITION;
};

struct PSPostProcessIn
{
	float4 Pos			: SV_POSITION;
	float2 Tex0			: TEXCOORD0;
};

Texture2D    g_txColor : register( t0 );
Texture2D    g_txVelocity : register( t1 );

SamplerState g_samPoint : register( s0 );
SamplerState g_samLinear : register( s1 );

//-----------------------------------------------------------------------------------------
// VertexShader for most post process
//-----------------------------------------------------------------------------------------
PSPostProcessIn VSPostProcess(VSPostProcessIn input)
{
	PSPostProcessIn output;
	
	output.Pos = input.Pos;
	output.Tex0.x =   (input.Pos.x+1.0f) / 2.0f;
	output.Tex0.y = - (input.Pos.y-1.0f) / 2.0f;

	return output;
}



//-----------------------------------------------------------------------------------------
// PixelShader for motion blur
//-----------------------------------------------------------------------------------------
float4 PSMotionBlur(PSPostProcessIn input) : SV_TARGET
{
	float2 velocity = g_txVelocity.Sample( g_samPoint, input.Tex0.xy ).xy;
	float maxSpeed = 0.02f;
	velocity = min( velocity, maxSpeed );
	velocity = max( velocity, -maxSpeed );

	// NumIterations controls the number of loop iterations and samples we take along the motion
	// blur vector
	uint NumIterations = 11;
	velocity /= NumIterations;
	
	float2 samplePos = input.Tex0.xy;
	float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float factor = 1.0f;
	float factorDecay = factor / NumIterations;
	float factorSum = 0.0f;
	[unroll] for( uint i = 0; i < NumIterations; ++i )
	{
		color += factor * g_txColor.Sample( g_samLinear, samplePos );
		factorSum += factor;
		samplePos -= velocity;
		factor -= factorDecay;

		//to prevent halo's around disperate velocity parts, we re-sample the velocity the velocity at the new position
		velocity = g_txVelocity.Sample( g_samPoint, samplePos ).xy;

		velocity = min( velocity, maxSpeed );
		velocity = max( velocity, -maxSpeed );
		velocity /= NumIterations;
	}
	color = color / factorSum;

	return color;	
	
}


struct PSClearInput
{
	float4 Pos			: SV_POSITION;
};

struct PSClearOutput
{
    float4 Color			: SV_TARGET0; 
    float2 Velocity			: SV_TARGET1;
};

//-----------------------------------------------------------------------------------------
// VertexShader for pixel shader clear
//-----------------------------------------------------------------------------------------
PSClearInput VSClear(VSPostProcessIn input)
{
	PSClearInput output;
	
	output.Pos = input.Pos;

	return output;
}

//-----------------------------------------------------------------------------------------
// PixelShader for pixel shader clear
//-----------------------------------------------------------------------------------------
PSClearOutput PSClear(PSClearInput input)
{
	PSClearOutput output;
	output.Color		= float4( 0.0f, 0.1f, 0.1f, 1.0f );
	output.Velocity		= float2( 0.0f, 0.0f );
	return output;
}