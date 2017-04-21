/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#include "SDKMeshExt.h"


//--------------------------------------------------------------------------------------
// CTor
//--------------------------------------------------------------------------------------
CDXUTSDKMeshExt::CDXUTSDKMeshExt()
	: m_pbFrameOfMatrix( NULL )
{
}


//--------------------------------------------------------------------------------------
// DTor
//--------------------------------------------------------------------------------------
CDXUTSDKMeshExt::~CDXUTSDKMeshExt()
{
	delete[] m_pbFrameOfMatrix;
}

//--------------------------------------------------------------------------------------
// Transform the mesh with linear interpolation between animation frames
// - does not support skinned meshes (support could be added)
//--------------------------------------------------------------------------------------
void CDXUTSDKMeshExt::TransformMeshWithInterpolation( D3DXMATRIX* pWorld, double fTime )
{
	TransformFrameWithInterpolation( 0, pWorld, fTime );
}

//--------------------------------------------------------------------------------------
// Transform the mesh frames along with their siblings and children,
// using linearly interpolated animation frames for smoother animation and motion blur
//--------------------------------------------------------------------------------------
void CDXUTSDKMeshExt::TransformFrameWithInterpolation( UINT iFrame, D3DXMATRIX* pParentWorld, double fTime )
{

	D3DXMATRIX transform;


    if( m_pAnimationHeader && INVALID_ANIMATION_DATA != m_pFrameArray[iFrame].AnimationDataIndex )
    {
		// Calculate previous and current keys, and interpolant between the,
		double keyFrameNumActual = m_pAnimationHeader->AnimationFPS * fTime;
		double KeyFrameNumPrev = floor( keyFrameNumActual );
		UINT iTickPrev = UINT( KeyFrameNumPrev + 0.1 ) % m_pAnimationHeader->NumAnimationKeys; // keyFrameNumPrev is integer, but need to add small value to get correct rounding
		UINT iTickNext = ( iTickPrev + 1 ) % m_pAnimationHeader->NumAnimationKeys;
		float interpolant = (float)( keyFrameNumActual - KeyFrameNumPrev );

        SDKANIMATION_FRAME_DATA* pAnimFrameData = &m_pAnimationFrameData[ m_pFrameArray[iFrame].AnimationDataIndex ];
        SDKANIMATION_DATA* pDataPrev = &pAnimFrameData->pAnimationData[ iTickPrev ];
        SDKANIMATION_DATA* pDataNext = &pAnimFrameData->pAnimationData[ iTickNext ];

        // Interpolate the positions based on interpolant
		D3DXVECTOR3 translatePos = ( 1.0f - interpolant ) * pDataPrev->Translation + interpolant * pDataNext->Translation;
        D3DXMATRIX translation;
        D3DXMatrixTranslation( &translation, translatePos.x, translatePos.y, translatePos.z );

		// Calulate and interpolate rotations using quaternions
        D3DXQUATERNION quatPrev;
        quatPrev.w = pDataPrev->Orientation.w;
        quatPrev.x = pDataPrev->Orientation.x;
        quatPrev.y = pDataPrev->Orientation.y;
        quatPrev.z = pDataPrev->Orientation.z;
        if( 0 == quatPrev.w  && 0 == quatPrev.x && 0 == quatPrev.y && 0 == quatPrev.z )
		{
            D3DXQuaternionIdentity( &quatPrev );
		}
		else
		{
			D3DXQuaternionNormalize( &quatPrev, &quatPrev );
		}

		D3DXQUATERNION quatNext;
        quatNext.w = pDataNext->Orientation.w;
        quatNext.x = pDataNext->Orientation.x;
        quatNext.y = pDataNext->Orientation.y;
        quatNext.z = pDataNext->Orientation.z;
        if( 0 == quatNext.w  && 0 == quatNext.x && 0 == quatNext.y && 0 == quatNext.z )
 		{
            D3DXQuaternionIdentity( &quatNext );
		}
		else
		{
			D3DXQuaternionNormalize( &quatNext, &quatNext );
		}

		D3DXQUATERNION quat;
		D3DXQuaternionSlerp( &quat, &quatPrev, &quatNext, interpolant );

        D3DXMATRIX rotation;
        D3DXMatrixRotationQuaternion( &rotation, &quat );

		// Interpolate scaling
 		D3DXVECTOR3 scaleVector = ( 1.0f - interpolant ) * pDataPrev->Scaling + interpolant * pDataNext->Scaling;
        D3DXMATRIX scale;
        D3DXMatrixScaling( &scale, scaleVector.x, scaleVector.y, scaleVector.z );

		// Calculate transform from scaling, rotation and translation
        transform = ( scale * rotation * translation  );

		 // Calculate resultant transform from parent
		D3DXMatrixMultiply( &transform, &transform, pParentWorld );
   }
    else
    {
 		D3DXMatrixMultiply( &transform, &(m_pFrameArray[iFrame].Matrix), pParentWorld );
    }


	// Store results
	m_pTransformedFrameMatrices[iFrame] = transform;
	m_pWorldPoseFrameMatrices[iFrame] = transform;


    // Recursively transform sibling frames
    if( INVALID_FRAME != m_pFrameArray[iFrame].SiblingFrame  )
	{
        TransformFrameWithInterpolation( m_pFrameArray[iFrame].SiblingFrame, pParentWorld, fTime );
	}

    // Recursively transform children
    if( INVALID_FRAME != m_pFrameArray[iFrame].ChildFrame  )
	{
        TransformFrameWithInterpolation( m_pFrameArray[iFrame].ChildFrame, &(m_pWorldPoseFrameMatrices[iFrame]), fTime );
	}
}


//--------------------------------------------------------------------------------------
// Updating shader constants potentially expensive, so we
// check if consequtive frames have the same matrix
// might be better to collapse frames, but this is simpler
// must call TransformFrame before hand
// only checks those meshes with frames
//--------------------------------------------------------------------------------------
void CDXUTSDKMeshExt::CheckForRedundantMatrices( float epsilon )
{
	if( m_pAnimationHeader )
    {
		// if we have an animation header we assume that the mesh animation means
		// frame matrices could change, so this is not true
		return;
	}

	if( 0 == GetNumFrames() )
	{
		// in this case there's no information
		return;
	}

	delete[] m_pbFrameOfMatrix;
	m_pbFrameOfMatrix = new UINT[ GetNumFrames() ];

	// Test matrices
	const D3DXMATRIX* pLastNonRedundantMatrix = 0;
	UINT lastNonRedundantFrame = 0;
	for( UINT frame = 0; frame < GetNumFrames(); ++frame )
	{
		m_pbFrameOfMatrix[frame] = frame;
		if( INVALID_MESH != GetFrame( frame )->Mesh )
		{
			const D3DXMATRIX* pWorldMat = GetWorldMatrix( frame );
			if( 0 == pLastNonRedundantMatrix  )
			{
				pLastNonRedundantMatrix = pWorldMat;
				lastNonRedundantFrame = frame;
				continue;
			}

			m_pbFrameOfMatrix[frame] = lastNonRedundantFrame;

			// slow almost equals comparison - would be faster as two's complement integer fast comparison
			// but this one reads easier, and works for current sample content
			for( int i = 0; i < 16; ++i )
			{
				float difference = (*pWorldMat)[i] - (*pLastNonRedundantMatrix)[i]; 
				if( 0.0f != difference && fabs( difference ) > epsilon )
				{
					// different
					m_pbFrameOfMatrix[frame] = frame;
					lastNonRedundantFrame = frame;
					pLastNonRedundantMatrix = pWorldMat;
					break;
				}
			}

		}
	}


}
