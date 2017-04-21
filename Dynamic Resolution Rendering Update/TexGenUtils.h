/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "DXUT.h"

//--------------------------------------------------------------------------------------
// Fast Cubic Filter lookup table, based on article:
// "Fast Third-Order Texture Filtering", GPU Gems 2, Christian Sigg, Markus Hadwiger
// Data size is sizeof( D3DXVECTOR4 ) * size.
//
// Currently creates a 32bit table, an optimization would be to create a 16bit version
//--------------------------------------------------------------------------------------
class CubicFilterLookupTable
{
public:
	CubicFilterLookupTable( unsigned int size );
	~CubicFilterLookupTable();

	const D3DXVECTOR4* GetTablePointer() const
	{
		return m_pData;
	}
private:
	D3DXVECTOR4* m_pData;

	//prevent assign and copy
	CubicFilterLookupTable( const CubicFilterLookupTable& rhs );
	CubicFilterLookupTable& operator=( const CubicFilterLookupTable& rhs );
};

//--------------------------------------------------------------------------------------
// Noise Texture, RGBA 8888. Data size is 4*size*size in bytes
// See rand with srand() prior to calling if required
//--------------------------------------------------------------------------------------
class NoiseTexture
{
public:
	NoiseTexture( unsigned int size );
	~NoiseTexture();

	const unsigned char* GetPointer() const
	{
		return m_pData;
	}

private:
	unsigned char* m_pData;

	//prevent assign and copy
	NoiseTexture( const NoiseTexture& rhs );
	NoiseTexture& operator=( const NoiseTexture& rhs );

};