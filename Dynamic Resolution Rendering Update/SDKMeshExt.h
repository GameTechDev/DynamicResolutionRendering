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

