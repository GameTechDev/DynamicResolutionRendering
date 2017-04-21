/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#include "DynamicResolution.h"

//--------------------------------------------------------------------------------------
// Set up default values
//--------------------------------------------------------------------------------------
DynamicResolution::DynamicResolution()
	: m_BackBufferWidth( 1280 )
	, m_BackBufferHeight( 720 )
	, m_MaxWidth( 2048 )
	, m_MaxHeight( 2048 )
	, m_MaxScalingX( 1.0f )
	, m_MaxScalingY( 1.0f )
	, m_DynamicRTWidth( 1280 )
	, m_DynamicRTHeight( 720 )
	, m_CurrDynamicRTWidth( 1280 )
	, m_CurrDynamicRTHeight( 720 )

{
}


//--------------------------------------------------------------------------------------
// Initializes parameters - seperate from constructor so the application can call
// this to reset on backbuffer changes (e.g. in OnD3D11ResizedSwapChain() ).
//--------------------------------------------------------------------------------------
void DynamicResolution::InitializeResolutionParameters( UINT backBufferWidth, UINT backBufferHeight, UINT maxWidth, UINT maxHeight, UINT maxScaling ) 
{
	m_BackBufferWidth	= backBufferWidth;
	m_BackBufferHeight	= backBufferHeight;
	m_MaxWidth			= maxWidth;
	m_MaxHeight			= maxHeight;
 
	// Validate parameters
	UINT maxScaleWidth = maxWidth / backBufferWidth;
	if( maxScaleWidth > 1 )
	{
		if( maxScaleWidth > maxScaling )
		{
			maxScaleWidth = maxScaling;
		}
		m_DynamicRTWidth = maxScaleWidth *  backBufferWidth;
	}
	else
	{
		m_DynamicRTWidth = backBufferWidth;
	}
	m_MaxScalingX = m_DynamicRTWidth / (float)backBufferWidth;

	UINT maxScaleHeight = maxHeight / backBufferHeight;
	if( maxScaleHeight > 1 )
	{
		if( maxScaleHeight > maxScaling )
		{
			maxScaleHeight = maxScaling;
		}
		m_DynamicRTHeight = maxScaleHeight *  backBufferHeight;
	}
	else
	{
		m_DynamicRTHeight = backBufferHeight;
	}
	m_MaxScalingY = m_DynamicRTHeight / (float)backBufferHeight;

	// Call setscale to initialize scale driven params
	SetScale( m_ScaleX, m_ScaleY );

}

//--------------------------------------------------------------------------------------
// Set the scale of the dynamic resolution in relation to the backbuffer
//--------------------------------------------------------------------------------------
void DynamicResolution::SetScale( float scaleX, float scaleY )
{
	if( scaleX < m_MaxScalingX )
	{
		m_ScaleX = scaleX;
	}
	else
	{
		m_ScaleX = m_MaxScalingX;
	}

	if( scaleY < m_MaxScalingY )
	{
		m_ScaleY = scaleY;
	}
	else
	{
		m_ScaleY = m_MaxScalingY;
	}

	// Now convert to give integer height and width for viewport
	m_CurrDynamicRTHeight	= floor( (float)m_DynamicRTHeight * m_ScaleY / m_MaxScalingY );
	m_CurrDynamicRTWidth	= floor( (float)m_DynamicRTWidth *  m_ScaleX / m_MaxScalingX );

	// Recreate scale values from actual viewport values
	m_ScaleY = m_CurrDynamicRTHeight * m_MaxScalingY / (float)m_DynamicRTHeight;
	m_ScaleX = m_CurrDynamicRTWidth *  m_MaxScalingX / (float)m_DynamicRTWidth;

}

//--------------------------------------------------------------------------------------
// Get a viewport constrained to the currently set dynamic resolution values
//--------------------------------------------------------------------------------------
void DynamicResolution::GetViewport( D3D11_VIEWPORT* pViewPort ) const
{
	if( pViewPort )
	{
		pViewPort->Height	= m_CurrDynamicRTHeight;
		pViewPort->Width	= m_CurrDynamicRTWidth;
		pViewPort->TopLeftX = 0.0f;
		pViewPort->TopLeftY = 0.0f;
		pViewPort->MinDepth = 0.0f;
		pViewPort->MaxDepth = 1.0f;
	}
}
