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
#include "GPUTimer.h"

using namespace GPUTimerConsts;

//--------------------------------------------------------------------------------------
// Ctor
//--------------------------------------------------------------------------------------
GPUTimer::GPUTimer()
	: m_QueryPipelineBegin( 0 )
	, m_NumQuerriesIssued( 0 )
	, m_bOpenInterval( false )
{
	for( size_t i=0; i < cPipelineDepth; ++ i )
	{
		m_pStartQueries[i] = 0; 
		m_pStopQueries[i]  = 0; 
		m_pFreqQueries[i]  = 0; 
	}

}

//--------------------------------------------------------------------------------------
// Dtor
//--------------------------------------------------------------------------------------
GPUTimer::~GPUTimer()
{
	OnD3D11DestroyDevice();
}


//--------------------------------------------------------------------------------------
// Query creation on device creation
//--------------------------------------------------------------------------------------
HRESULT GPUTimer::OnD3D11CreateDevice(ID3D11Device* pD3DDevice, ID3D11DeviceContext* pImmediateContext)
{
	HRESULT hr = S_OK;
	D3D11_QUERY_DESC timerQueryDesc;
	timerQueryDesc.Query = D3D11_QUERY_TIMESTAMP;
	timerQueryDesc.MiscFlags = 0;
	D3D11_QUERY_DESC freqQueryDesc;
	freqQueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	freqQueryDesc.MiscFlags = 0;

	for( size_t i=0; i < cPipelineDepth; ++ i )
	{
		V_RETURN( pD3DDevice->CreateQuery( &timerQueryDesc, &( m_pStartQueries[i] ) ) );
		V_RETURN( pD3DDevice->CreateQuery( &timerQueryDesc, &( m_pStopQueries[i] ) ) );
		V_RETURN( pD3DDevice->CreateQuery( &freqQueryDesc,  &( m_pFreqQueries[i] ) ) );
	}
	return hr;
}

//--------------------------------------------------------------------------------------
// Query delete on device destruction
//--------------------------------------------------------------------------------------
void    GPUTimer::OnD3D11DestroyDevice()
{
	m_QueryPipelineBegin = 0;
	m_NumQuerriesIssued  = 0;
	m_bOpenInterval      =  false;

	for( size_t i=0; i < cPipelineDepth; ++ i )
	{
		SAFE_RELEASE( m_pStartQueries[i] ); 
		SAFE_RELEASE( m_pStopQueries[i] ); 
		SAFE_RELEASE( m_pFreqQueries[i] ); 
	}
}

//--------------------------------------------------------------------------------------
// Begin timer, call this at the start of the block of D3D calls you want GPU timings for
// Use only one begin/end pair per frame and use more GPUTimers if you want to query
// more than one timing interval per frame.
//--------------------------------------------------------------------------------------
bool GPUTimer::Begin( ID3D11DeviceContext* pImmediateContext )
{

	if( m_bOpenInterval || m_NumQuerriesIssued >= cPipelineDepth )
	{
		return false;
	}
	size_t queryToIssue = ( m_NumQuerriesIssued + m_QueryPipelineBegin ) % cPipelineDepth;

	// must issue frequency and disjoint query first to ensure we have valid data over interval
	pImmediateContext->Begin( m_pFreqQueries[ queryToIssue ] );

	// timestamp queries issued and ended immediatly
	pImmediateContext->End( m_pStartQueries[ queryToIssue ] );
	m_bOpenInterval = true;
	return true;
}

//--------------------------------------------------------------------------------------
// End timer, call this at the end of the block of D3D calls you want GPU timings for
//--------------------------------------------------------------------------------------
bool GPUTimer::End( ID3D11DeviceContext* pImmediateContext )
{
	if( !m_bOpenInterval )
	{
		return false;
	}

	size_t queryToIssue = ( m_NumQuerriesIssued + m_QueryPipelineBegin ) % cPipelineDepth;

	// timestamp queries issued and ended immediatly
	pImmediateContext->End( m_pStopQueries[ queryToIssue ] );

	// now issue frequency and disjoint query end to ensure we have valid data over interval
	pImmediateContext->End( m_pFreqQueries[ queryToIssue ] );


	++m_NumQuerriesIssued;
	m_bOpenInterval = false;
	return true;
}

//--------------------------------------------------------------------------------------
// Returns true if there is a valid interval, and stores the interval
// between begin/end pairs for the first available result in
// pInterval and pFreq (interval time = *pInterval/*pFreq)
// and then removes that result from the pipeline.
// Standard usage would be to call this every frame, storing the result the return
// value is true.
//--------------------------------------------------------------------------------------
bool GPUTimer::GetInterval( ID3D11DeviceContext* pImmediateContext, UINT64* pInterval, UINT64* pFreq )
{
	*pInterval = (UINT64)0;
	*pFreq = (UINT64)0;

	if( m_NumQuerriesIssued && !m_bOpenInterval )
	{
		// all queries must succeed for this to be valid
		UINT64 queryDataStart;
		HRESULT hr = pImmediateContext->GetData( m_pStartQueries[ m_QueryPipelineBegin ], &queryDataStart, sizeof(UINT64), D3D11_ASYNC_GETDATA_DONOTFLUSH );
		if( hr != S_OK ) { return false; }

		UINT64 queryDataStop;
		hr = pImmediateContext->GetData( m_pStopQueries[ m_QueryPipelineBegin ], &queryDataStop, sizeof(UINT64), D3D11_ASYNC_GETDATA_DONOTFLUSH );
		if( hr != S_OK ) { return false; }

		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT queryDataTSD;
		hr = pImmediateContext->GetData( m_pFreqQueries[ m_QueryPipelineBegin ], &queryDataTSD, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), D3D11_ASYNC_GETDATA_DONOTFLUSH );
		if( hr != S_OK ) { return false; }

		// have a potentially valid set of data, so increment
		++m_QueryPipelineBegin;
		if( m_QueryPipelineBegin >= cPipelineDepth )
		{
			m_QueryPipelineBegin = 0;
		}
		--m_NumQuerriesIssued;

		if( queryDataStop > queryDataStart )
		{
			*pInterval	= queryDataStop - queryDataStart;
		}
		else
		{
			*pInterval	= 0;
		}
		*pFreq		= queryDataTSD.Frequency;
		return !queryDataTSD.Disjoint;
	}

	return false;
}



//--------------------------------------------------------------------------------------
// Ctor
//--------------------------------------------------------------------------------------
AveragedGPUTimer::AveragedGPUTimer( size_t updateCount  )
	: m_UpdateCount( 10 )
	, m_AveragedIntervalTime( 0.0f )
	, m_AccumulatingIntervalTime( 0.0f )
	, m_NumAccumulations( 0 )

{
}

//--------------------------------------------------------------------------------------
// Dtor
//--------------------------------------------------------------------------------------
AveragedGPUTimer::~AveragedGPUTimer()
{
}


//--------------------------------------------------------------------------------------
// Updates averaged interval time to pAveragedIntervalTime
//--------------------------------------------------------------------------------------
bool AveragedGPUTimer::HaveUpdatedAveragedIntervalTime( ID3D11DeviceContext* pImmediateContext, float* pAveragedIntervalTime )
{
	bool bUpdate = false;
	UINT64 interval, freq;
	bool bValidInterval = GetInterval( pImmediateContext, &interval, &freq );
	if( bValidInterval )
	{
		//update timer
		double time = (double)interval/(double)freq;
		m_AccumulatingIntervalTime += (float)time;
		++m_NumAccumulations;
		if( m_NumAccumulations >= m_UpdateCount )
		{
			m_AveragedIntervalTime = m_AccumulatingIntervalTime / (float)m_NumAccumulations;
			m_AccumulatingIntervalTime = 0.0f;
			m_NumAccumulations = 0;
			bUpdate = true;
		}

	}
	*pAveragedIntervalTime = m_AveragedIntervalTime;

	return bUpdate;
}
