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

cbuffer cbVSPostProcess : register( b0 )
{
	float4	g_VSSubSampleRTCurrRatio		: packoffset( c0 ); // float4( ratio_x, ratio_y, Not used, Not used )
};

cbuffer cbPSMotionBlur : register( b0 )
{
	float4	g_PSSubSampleRTCurrRatio		: packoffset( c0 ); // float4( ratio_x, ratio_y, Width/g_VelocityToAlphaScale, Height/g_VelocityToAlphaScale )
};

cbuffer cbPSResolveCubic : register( b0 )
{
	float2	g_texsize_x		: packoffset( c0 );		// float2( 1/width, 0 )
	float2	g_texsize_y		: packoffset( c0.z );	// float2( 0, 1/height )
	float4	g_SizeSource	: packoffset( c1 );		// float4( width, height, Not used, Not used )
};

cbuffer cbPSResolveNoise : register( b0 )
{
	float2	g_NoiseTexScale	: packoffset( c0 );		// float2( scale.x, scale.y )
	float2	g_NoiseOffset	: packoffset( c0.z );	// float2( offset.x, offset.x )
	float4	g_NoiseScale	: packoffset( c1 );		// float4( width, height, Not used, Not used )
};

Texture2D    g_txColor				: register( t0 );
Texture2D    g_txVelocity			: register( t1 );
Texture1D    g_txCubicLookupFilter	: register( t1 );
Texture2D    g_txNoise				: register( t1 );

SamplerState g_samPoint				: register( s0 );
SamplerState g_samLinear			: register( s1 );

SamplerState g_samResolve			: register( s0 );
SamplerState g_samLinearWrap		: register( s1 );

//-----------------------------------------------------------------------------------------
// VertexShader
//-----------------------------------------------------------------------------------------
PSPostProcessIn VSPostProcess(VSPostProcessIn input)
{
	PSPostProcessIn output;
	
	output.Pos = input.Pos;
	output.Tex0.x =   g_VSSubSampleRTCurrRatio.x * (input.Pos.x+1.0f) / 2.0f;
	output.Tex0.y =   -g_VSSubSampleRTCurrRatio.y * (input.Pos.y-1.0f) / 2.0f;

	return output;
}


//-----------------------------------------------------------------------------------------
// PixelShader for motion blur
//-----------------------------------------------------------------------------------------
float4 PSMotionBlur(PSPostProcessIn input) : SV_TARGET
{
	float2 velocity = g_txVelocity.Sample( g_samPoint, input.Tex0.xy ).xy;
	float2 tempVelocityForAlpha = velocity * g_PSSubSampleRTCurrRatio.zw;


	float maxSpeed = 0.02f;
	velocity = min( velocity, maxSpeed );
	velocity = max( velocity, -maxSpeed );


	// Scale and clamp velocity to texture size
	velocity *= g_PSSubSampleRTCurrRatio.xy;
	float2 clampedPos = min( input.Tex0.xy  - velocity, g_PSSubSampleRTCurrRatio.xy );
	velocity = -( clampedPos - input.Tex0.xy );

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

		// Scale and clamp velocity to texture size
		velocity *= g_PSSubSampleRTCurrRatio.xy;
		float2 clampedPos = min( samplePos  - velocity, g_PSSubSampleRTCurrRatio.xy );
		velocity = -( clampedPos - samplePos );
	}
	color = color / factorSum;
	color.a = dot( tempVelocityForAlpha, tempVelocityForAlpha ); //for Fast Temporal AA
	return color;
	
}

//-----------------------------------------------------------------------------------------
// PixelShader for resolve (point and bilinear)
//-----------------------------------------------------------------------------------------
float4 PSResolve(PSPostProcessIn input) : SV_TARGET
{

	float4 color = g_txColor.Sample( g_samResolve, input.Tex0.xy );
	return color;
	
}

//-----------------------------------------------------------------------------------------
// PixelShader for bicubic resolve
//-----------------------------------------------------------------------------------------
float4 PSResolveCubic(PSPostProcessIn input) : SV_TARGET
{

	// calc filter texture coords
	float2 coord_hg = input.Tex0.xy * g_SizeSource.xy -  float2( 0.5f, 0.5f);

	// fetch offsets and weights
	float3 hg_x = g_txCubicLookupFilter.Sample( g_samLinearWrap, coord_hg.x ).xyz; 
	float3 hg_y = g_txCubicLookupFilter.Sample( g_samLinearWrap, coord_hg.y ).xyz; 
	hg_x.xy *= 0.25f;
	hg_y.xy *= 0.25f;

	// form a square of sample points distributed around the sample
	//  01 11
	//    X
	//	00 10
	float2 coord_source10 = input.Tex0.xy + hg_x.x * g_texsize_x;
	float2 coord_source00 = input.Tex0.xy - hg_x.y * g_texsize_x;

	float2 coord_source11 = coord_source10 + hg_y.x * g_texsize_y;
	float2 coord_source01 = coord_source00 + hg_y.x * g_texsize_y;

	coord_source10 = coord_source10 - hg_y.y * g_texsize_y;
	coord_source00 = coord_source00 - hg_y.y * g_texsize_y;

	// now sample the texture at the four locations
	float4 color00 = g_txColor.Sample( g_samResolve, coord_source00 );
	float4 color01 = g_txColor.Sample( g_samResolve, coord_source01 );
	float4 color10 = g_txColor.Sample( g_samResolve, coord_source10 );
	float4 color11 = g_txColor.Sample( g_samResolve, coord_source11 );

	// lerp along Y direction
	color00 = lerp( color00, color01, hg_y.z );
	color10 = lerp( color10, color11, hg_y.z );


	// lerp along X direction
	color00 = lerp( color00, color10, hg_x.z );

	return color00;
}

//-----------------------------------------------------------------------------------------
// PixelShader for noisy resolve
//-----------------------------------------------------------------------------------------
float4 PSResolveNoise(PSPostProcessIn input) : SV_TARGET
{
	float4 color = g_txColor.Sample( g_samResolve, input.Tex0.xy );
	float4 noise = g_txNoise.Sample( g_samLinearWrap, input.Tex0.xy * g_NoiseTexScale + g_NoiseOffset );

	noise = 1.0f + ( ( noise - 0.5f ) * g_NoiseScale.x );	//scale noise around 1.0f
	color = color * noise;
	return color;
}

//-----------------------------------------------------------------------------------------
// PixelShader for noisy resolve
//-----------------------------------------------------------------------------------------
float4 PSResolveNoiseOffset(PSPostProcessIn input) : SV_TARGET
{
	float4 noise = g_txNoise.Sample( g_samLinearWrap, input.Tex0.xy * g_NoiseTexScale );

	float2 newTex = input.Tex0.xy + 2.0f * ( noise.xy - 0.5f ) * g_NoiseScale;
	float4 color = g_txColor.Sample( g_samResolve, newTex.xy );
	return color;
}



//-----------------------------------------------------------------------------------------
// Temporal Anti-alias resolve
//-----------------------------------------------------------------------------------------
struct PSPostProcessTemporalAAIn
{
	float4 Pos			: SV_POSITION;
	float2 Tex0			: TEXCOORD0;
	float2 Tex1			: TEXCOORD1;
};

cbuffer cbVSPostProcess : register( b0 )
{
	float2	g_VSRTCurrRatio0		: packoffset( c0 ); // float4( ratio_x, ratio_y )
	float2	g_VSOffset0	    	    : packoffset( c0.z ); // float4( offset_x, offset_y )
	float2	g_VSRTCurrRatio1		: packoffset( c1 ); // float4( ratio_x, ratio_y )
	float2	g_VSOffset1    		    : packoffset( c1.z ); // float4( offset_x, offset_y )
};

cbuffer cbVSPostProcess : register( b1 )
{
	float2  g_VelocityScale			: packoffset( c0 );
	float2	g_VelocityAlphaScale 	: packoffset( c0.z );
};

Texture2D    g_txColor0				: register( t0 );
Texture2D    g_txColor1				: register( t1 );
Texture2D    g_txVelocityTAA0 		: register( t2 );
Texture2D    g_txVelocityTAA1		: register( t3 );
Texture2D    g_txNoiseTAA			: register( t4 );

//-----------------------------------------------------------------------------------------
// VertexShader for Temporal Anti-alias resolve
//-----------------------------------------------------------------------------------------
PSPostProcessTemporalAAIn VSPostProcessTemporalAA(VSPostProcessIn input)
{
	PSPostProcessTemporalAAIn output;
	
	output.Pos = input.Pos;
	output.Tex0.x =   g_VSRTCurrRatio0.x * ( (input.Pos.x+1.0f) / 2.0f + g_VSOffset0.x );
	output.Tex0.y =   -g_VSRTCurrRatio0.y * ( (input.Pos.y-1.0f) / 2.0f + g_VSOffset0.y );

	output.Tex1.x =   g_VSRTCurrRatio1.x * ( (input.Pos.x+1.0f) / 2.0f + g_VSOffset1.x );
	output.Tex1.y =   -g_VSRTCurrRatio1.y * ( (input.Pos.y-1.0f) / 2.0f + g_VSOffset1.y );

	return output;
}

//-----------------------------------------------------------------------------------------
// PixelShader for Temporal Anti-alias resolve
//-----------------------------------------------------------------------------------------
float4 PSResolveTemporalAA(PSPostProcessTemporalAAIn input) : SV_TARGET
{

	float4 color = g_txColor0.Sample( g_samResolve, input.Tex0.xy );

	float2 velocity0 = g_VelocityScale * g_txVelocityTAA0.Sample( g_samResolve, input.Tex0.xy ).xy;
	float2 velocity1 = g_VelocityScale * g_txVelocityTAA1.Sample( g_samResolve, input.Tex1.xy ).xy;

	// we sample the previous RT to get Temporal AA but with a velocity dependant factor
	float prevFactor = 1.0f / ( 1.0f + dot( velocity0, velocity0 ) + dot( velocity1, velocity1 ) );
	color        += prevFactor * g_txColor1.Sample( g_samResolve, input.Tex1.xy);
	return color / (1.0f + prevFactor );
}

//-----------------------------------------------------------------------------------------
// PixelShader for Fast Temporal Anti-alias resolve
//-----------------------------------------------------------------------------------------
float4 PSResolveTemporalAAFast(PSPostProcessTemporalAAIn input) : SV_TARGET
{

	float4 color = g_txColor0.Sample( g_samResolve, input.Tex0.xy );
	float4 colorOld = g_txColor1.Sample( g_samResolve, input.Tex1.xy);


	// we sample the previous RT to get Temporal AA but with a velocity dependant factor
	float prevFactor = 1.0f / ( 1.0f + g_VelocityAlphaScale.x * ( color.a + colorOld.a ) );
	color        += prevFactor * colorOld;
	return color / (1.0f + prevFactor );
}

//-----------------------------------------------------------------------------------------
// PixelShader for Temporal Anti-alias resolve with Noise Offset
//-----------------------------------------------------------------------------------------
float4 PSResolveVelTemporalAANoiseOffset(PSPostProcessTemporalAAIn input) : SV_TARGET
{
	// sample noise at combination of both texture coordinates to get consistant sample,
	// irrespective of which coordinate is jittered
	float4 noise = g_txNoiseTAA.Sample( g_samLinearWrap, ( input.Tex0.xy + input.Tex1.xy ) * g_NoiseTexScale );

	float2 offset = 2.0f * ( noise.xy - 0.5f ) * g_NoiseScale;

	float4 color = g_txColor0.Sample( g_samResolve, input.Tex0.xy + offset );

	float2 velocity0 = g_VelocityScale * g_txVelocityTAA0.Sample( g_samResolve, input.Tex0.xy + offset ).xy;
	float2 velocity1 = g_VelocityScale * g_txVelocityTAA1.Sample( g_samResolve, input.Tex1.xy + offset ).xy;

	// we sample the previous RT to get Temporal AA but with a velocity dependant factor
	float prevFactor = 1.0f / ( 1.0f + dot( velocity0, velocity0 ) + dot( velocity1, velocity1 ) );
	color        += prevFactor * g_txColor1.Sample( g_samResolve, input.Tex1.xy + offset);
	return color / (1.0f + prevFactor );
}

//-----------------------------------------------------------------------------------------
// PixelShader for Fast Temporal Anti-alias resolve with Noise Offset
//-----------------------------------------------------------------------------------------
float4 PSResolveVelTemporalAANoiseOffsetFast(PSPostProcessTemporalAAIn input) : SV_TARGET
{
	// sample noise at combination of both texture coordinates to get consistant sample,
	// irrespective of which coordinate is jittered
	float4 noise = g_txNoiseTAA.Sample( g_samLinearWrap, ( input.Tex0.xy + input.Tex1.xy ) * g_NoiseTexScale );

	float2 offset = 2.0f * ( noise.xy - 0.5f ) * g_NoiseScale;

	float4 color = g_txColor0.Sample( g_samResolve, input.Tex0.xy + offset );
	float4 colorOld = g_txColor1.Sample( g_samResolve, input.Tex1.xy + offset);

	// we sample the previous RT to get Temporal AA but with a velocity dependant factor
	float prevFactor = 1.0f / ( 1.0f + g_VelocityAlphaScale.x * ( color.a + colorOld.a ) );
	color        += prevFactor * colorOld;
	return color / (1.0f + prevFactor );
}

//-----------------------------------------------------------------------------------------
// PixelShader for Basic Temporal Anti-alias resolve
//-----------------------------------------------------------------------------------------
float4 PSResolveTemporalAABasic(PSPostProcessTemporalAAIn input) : SV_TARGET
{
	float4 color = g_txColor0.Sample( g_samResolve, input.Tex0.xy );
	color        +=  g_txColor1.Sample( g_samResolve, input.Tex1.xy);
	return color / 2.0f;
}