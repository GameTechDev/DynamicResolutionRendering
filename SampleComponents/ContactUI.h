/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");// you may not use this file except in compliance with the License.// You may obtain a copy of the License at//// http://www.apache.org/licenses/LICENSE-2.0//// Unless required by applicable law or agreed to in writing, software// distributed under the License is distributed on an "AS IS" BASIS,// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.// See the License for the specific language governing permissions and// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CONTACTUI_H
#define __CONTACTUI_H

#include <tchar.h>

#include "DXUT.h"
#include "SDKmisc.h"

#include "resource.h"

/// < summary >
///     Handles messages sent to dialog box.	
/// < /summary >
/// <param name="param1">
///		HWND - A handle to the dialog box. 
/// </param>
/// <param name="param2">
///		UINT - The message or dialog box ID.
/// </param>
/// <param name="param3">
///		WPARAM - Additional message-specific information. 
/// </param>
/// <param name="param4">
///		LPARAM - Additional message-specific information
/// </param>
BOOL CALLBACK   ShowMessage( HWND,
                             UINT,
                             WPARAM,
                             LPARAM );


/// < summary >
///     Handles messages sent to dialog box.	
/// < /summary >
/// <param name="param1">
///		TCHAR* - Pointer to string to hold version info 
/// </param>
/// <param name="param2">
///		UINT - Max buffer length of string
/// </param>
BOOL GetAppVersionString( TCHAR*,
                          UINT );

/// < summary >
///		Handles on click action on 'Contact Us' button.
///		When the 'Conatct Us' button is clicked, a dialog box pops up which displays
///		the link for the feedback emails and link to VCSE website.
/// < /summary >
void RenderContactBox();

#endif //__CONTACTUI_H

