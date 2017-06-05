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