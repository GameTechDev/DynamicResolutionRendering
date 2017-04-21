/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "dxut.h"

//--------------------------------------------------------------------------------------
// This class is used to hold dynamic resolution information.
//--------------------------------------------------------------------------------------
class DynamicResolution
{
public:
	DynamicResolution();

	void InitializeResolutionParameters( UINT backBufferWidth, UINT backBufferHeight, UINT maxWidth, UINT maxHeight, UINT maxScaling );
	void SetScale( float scaleX, float scaleY );

	// Accessors
	float GetScaleX() const
	{
		return m_ScaleX;
	}
	float GetScaleY() const
	{
		return m_ScaleY;
	}
	float GetRTScaleX() const
	{
		return m_ScaleX/m_MaxScalingX;
	}
	float GetRTScaleY() const
	{
		return m_ScaleY/m_MaxScalingY;
	}
	UINT GetDynamicBufferWidth() const
	{
		return m_DynamicRTWidth;
	}
	UINT GetDynamicBufferHeight() const
	{
		return m_DynamicRTHeight;
	}

	void GetViewport( D3D11_VIEWPORT* pViewPort ) const;

private:
	UINT m_BackBufferWidth;
	UINT m_BackBufferHeight;
	UINT m_MaxWidth;
	UINT m_MaxHeight;

	UINT m_DynamicRTWidth;
	UINT m_DynamicRTHeight;

	float m_CurrDynamicRTWidth;
	float m_CurrDynamicRTHeight;

	float m_ScaleX;
	float m_ScaleY;
	float m_MaxScalingX;
	float m_MaxScalingY;
};

