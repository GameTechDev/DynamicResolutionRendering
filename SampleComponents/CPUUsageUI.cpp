/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////

#include "CPUUsageUI.h"

/// < summary >
///	    CPUUSageUI class constructor
/// < /summary >
/// <param name="param1">
///		unsigned int - Number of Logical CPUs
/// </param>
CPUUsageUI::CPUUsageUI( unsigned int uNumCounters )
{
    //Update the CPU count 
    m_uNumCounters = uNumCounters;
    m_uNumCPUs = m_uNumCounters - 1;

    // If we are on a single CPU system without HT
    if( m_uNumCPUs <= 0 )
    {
        m_uNumCPUs = 1;
    }

    //Have we have already allocated the array to hold CPU stats?
    if( m_pdCPUPercent )
        delete [] m_pdCPUPercent;

    //Allocate array to hold the CPU stats
    m_pdCPUPercent = new double[m_uNumCounters];

    //Initialize the CPU stats array with 0
    for( unsigned int i = 0; i < m_uNumCounters; i++ )
    {
        m_pdCPUPercent[i] = 0.0f;
    }
}


/// < summary >
///	    CPUUSageUI class destructor
/// < /summary >
CPUUsageUI::~CPUUsageUI()
{
    //Delete the allocated CPU stats array
    if( m_pdCPUPercent )
        delete [] m_pdCPUPercent;
}


/// < summary >
///	    Update and render the CPU usage stats on the screen
/// < /summary >
/// <param name="param1">
///		CDXUTTextHelper* - Pointer to DXUTTextHelper to draw on screen
/// </param>
/// <param name="param2">
///		CPUUSage& - Reference to CPUUsage object to obtain the updated CPU stats array
/// </param>
void CPUUsageUI::RenderCPUUsage( CDXUTTextHelper* pTxtHelper,
                                 CPUUsage& aCPUUsage,
                                 int iX,
                                 int iY )
{
    WCHAR buffer[100];

    //Update the CPU stats and store in local array 
    aCPUUsage.UpdatePeriodicData();
    aCPUUsage.getCPUCounters( m_pdCPUPercent );

    //Print the total number of logical CPUs
    pTxtHelper->SetInsertionPos( iX, iY );
    swprintf_s( buffer, 100, L"Logical CPUs: %d", m_uNumCPUs );
    pTxtHelper->DrawFormattedTextLine( buffer );

    //Draw the per CPU usage on the screen
    for( unsigned int i = 0; i < m_uNumCPUs; i++ )
    {
        pTxtHelper->SetInsertionPos( iX, iY += 20 );
        swprintf_s( buffer, 100, L"CPU%d: %d", i, ( int )m_pdCPUPercent[i] );
        pTxtHelper->DrawFormattedTextLine( buffer );
    }	
}
