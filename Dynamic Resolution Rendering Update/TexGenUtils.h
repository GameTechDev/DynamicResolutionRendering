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