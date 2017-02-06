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

