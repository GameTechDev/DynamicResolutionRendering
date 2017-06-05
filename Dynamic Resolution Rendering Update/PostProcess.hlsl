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