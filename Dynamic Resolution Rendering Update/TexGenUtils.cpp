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
#include "TexGenUtils.h"
//#include <cstdlib>

//--------------------------------------------------------------------------------------
// CTor - initializes table.
//--------------------------------------------------------------------------------------
CubicFilterLookupTable::CubicFilterLookupTable( unsigned int size )
	: m_pData( NULL )
{
	m_pData = new D3DXVECTOR4[ size ];
	if( 0 == m_pData )
	{
		//no error handling in this function, calling function should handle this case
		return;
	}

	float step  = 1.0f / (float)size;
	float x = 0.0f;
	for( unsigned int i=0; i < size; ++i )
	{
		D3DXVECTOR4 W;
		float x2 = x*x;
		float x3 = x2*x;
		W.x = -x3 + 3.0f*x2 - 3.0f*x + 1.0f;
		W.y = 3.0f*x3 - 6.0f*x2 + 4.0f;
		W.z = -3.0f*x3 + 3.0f*x2 + 3.0f*x + 1.0f;
		W.w = x3;
		W *= 1.0f/6.0f;

		D3DXVECTOR4 HG;
		HG.z = W.x + W.y;
		HG.w = W.z + W.w;
		HG.x = 1.0f - W.y / HG.z + x;
		HG.y = 1.0f + W.w / HG.w - x;
		m_pData[ i ] = HG;
		x += step;
	}
}


//--------------------------------------------------------------------------------------
// DTor
//--------------------------------------------------------------------------------------
CubicFilterLookupTable::~CubicFilterLookupTable()
{
	delete[] m_pData;
}


//--------------------------------------------------------------------------------------
// CTor - initializes data.
//--------------------------------------------------------------------------------------
NoiseTexture::NoiseTexture( unsigned int size )
{
	unsigned int dataSize = 4 * size * size;
	m_pData = new unsigned char[ dataSize ];
	if( 0 == m_pData )
	{
		//no error handling in this function, calling function should handle this case
		return;
	}

	unsigned char* pEnd = m_pData + dataSize;
	unsigned char* pThis = m_pData;
	while( pThis != pEnd )
	{
		*pThis = static_cast<unsigned char>( 256.0f * static_cast<float>( rand() ) / RAND_MAX );
		++pThis;
	}
}

//--------------------------------------------------------------------------------------
// DTor
//--------------------------------------------------------------------------------------
NoiseTexture::~NoiseTexture()
{
	delete[] m_pData;
}