/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CPUUSAGE_H
#define __CPUUSAGE_H

#include <vector>
#include <windows.h>

#include "pdh.h"
#include "pdhmsg.h"
#include "tchar.h"

class CPUUsage
{
public:
    CPUUsage( void );
    ~CPUUsage();
	
    void UpdatePeriodicData();

    unsigned int getCPUCount()
    {
        return m_CPUCount;
    }
    unsigned int getNumCounters()
    {
        return m_numCounters;
    }
    void getCPUCounters( double* CPUPercent )
    {
        if( CPUPercent != NULL )
        {
            for( unsigned int i = 0; i < m_numCounters; i++ )
            {
                CPUPercent[i] = m_CPUPercentCounters[i];
            }
        }
        return;
    }

private:
    unsigned int m_CPUCount;
    unsigned int m_numCounters;
    double* m_CPUPercentCounters;
    float m_secondsPerUpdate;

    LARGE_INTEGER m_LastUpdateTick;
    LARGE_INTEGER m_Frequency;

    std::vector <void*> m_vecProcessorCounters;

    // Index of the performance object called "Processor" in English.
    // From registry, in HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\PerfLib\<your language ID>.
    static const int m_processorObjectIndex = 238;
	
    // Take a guess at how long the name could be in all languages of the "Processor" counter object.
    static const int m_processorObjectNameMaxSize = 128;
};

#endif
