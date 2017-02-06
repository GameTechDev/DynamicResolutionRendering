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

// Constants
namespace GPUTimerConsts
{
	const size_t cPipelineDepth = 4;
};

//--------------------------------------------------------------------------------------
// Basic GPUTimer implementing a query based GPU Event timing begin / end pair
// with a pipeline
//--------------------------------------------------------------------------------------
class GPUTimer
{
public:
	GPUTimer();
	~GPUTimer();

	// Issue a timestamp begin, if possible. Returns true if it succeeds
	// failure would indicate that all available queries have been issued
	// Must be issued in non interleaved pairs per timer
	bool Begin( ID3D11DeviceContext* pImmediateContext );

	// Issue a timestamp end to create an interval query
	bool End( ID3D11DeviceContext* pImmediateContext );

	// Get the interval between being/end queries in seconds
	// returns true if successful
	// interval is for the first outstanding query which has been issued
	bool GetInterval( ID3D11DeviceContext* pImmediateContext, UINT64* pInterval, UINT64* pFreq );

    HRESULT OnD3D11CreateDevice(ID3D11Device* pD3DDevice, ID3D11DeviceContext* pImmediateContext);
    void    OnD3D11DestroyDevice();
private:
	ID3D11Query*	m_pStartQueries[ GPUTimerConsts::cPipelineDepth ];
	ID3D11Query*	m_pStopQueries[ GPUTimerConsts::cPipelineDepth ];
	ID3D11Query*	m_pFreqQueries[ GPUTimerConsts::cPipelineDepth ];

	size_t			m_QueryPipelineBegin;	// begining of issued queries
	size_t			m_NumQuerriesIssued;	// number of issued queries
	bool			m_bOpenInterval;
};

//--------------------------------------------------------------------------------------
// Average value GPU timing based on GPUTimer used over multiple frames
//--------------------------------------------------------------------------------------
class AveragedGPUTimer : public GPUTimer
{
public:
	AveragedGPUTimer( size_t updateCount );
	~AveragedGPUTimer();

	// normal usuage is to call HaveUpdatedAveragedIntervalTime and update GUI etc. when this returns true
	bool HaveUpdatedAveragedIntervalTime( ID3D11DeviceContext* pImmediateContext, float* pAveragedIntervalTime );
private:

	size_t			m_UpdateCount;
	float			m_AveragedIntervalTime;
	float			m_AccumulatingIntervalTime;
	size_t			m_NumAccumulations;
};