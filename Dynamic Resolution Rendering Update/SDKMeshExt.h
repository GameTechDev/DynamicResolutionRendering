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
#include "SDKmesh.h"
#include "SDKmisc.h"

//--------------------------------------------------------------------------------------
// Extended version of SDKMesh supporting extra functionality, such as
// interpolated animation and correct animation scaling.
//
// This version only supports non skinned meshes.
//--------------------------------------------------------------------------------------
class CDXUTSDKMeshExt :
	public CDXUTSDKMesh
{
public:
	CDXUTSDKMeshExt();
	~CDXUTSDKMeshExt();


	void                            TransformMeshWithInterpolation( D3DXMATRIX* pWorld, double fTime );

	void							CheckForRedundantMatrices( float epsilon );

	//--------------------------------------------------------------------------------------
	// returns the frame for which this matrix is a redundant version of
	// need to call CheckForRedundantMatrices to initialise
	// returns current frame if not redundant
	// can use this frame to decide if a constant buffer update is required
	//--------------------------------------------------------------------------------------
	UINT							GetFrameOfMatrix( UINT frame ) const
	{
		if( m_pbFrameOfMatrix && frame < m_pMeshHeader->NumFrames )
		{
			return m_pbFrameOfMatrix[ frame ];
		}
		else
		{
			// have to return something, may as well be false
			return frame;
		}
	}

protected:
	void                            TransformFrameWithInterpolation( UINT iFrame, D3DXMATRIX* pParentWorld, double fTime );
	UINT*							m_pbFrameOfMatrix;

	//prevent assign and copy
	CDXUTSDKMeshExt( const CDXUTSDKMeshExt& rhs );
	CDXUTSDKMeshExt& operator=( const CDXUTSDKMeshExt& rhs );
};

