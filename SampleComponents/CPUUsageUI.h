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

#ifndef __CPUUSAGEUI_H
#define __CPUUSAGEUI_H

#include "DXUT.h"
#include "SDKMisc.h"

#include "CPUUsage.h"

class CPUUsageUI
{
public:
    //Initialize the constructor with the CPU count
    CPUUsageUI( unsigned int );
    ~CPUUsageUI();

    void RenderCPUUsage( CDXUTTextHelper*,
                         CPUUsage&,
                         int = 10,
                         int = 120 );

private:
    //Declare the default constructor as private
    CPUUsageUI();

    unsigned int m_uNumCounters;
    unsigned int m_uNumCPUs;
    double* m_pdCPUPercent;
};

#endif
