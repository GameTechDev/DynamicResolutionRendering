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

#include "HelpUI.h"

// Displays default help and key bindings
void RenderHelp( CDXUTTextHelper* pTxtHelper )
{	
    pTxtHelper->DrawTextLine( L"Controls:" );
	
    //Render shortcut keys.
    pTxtHelper->DrawTextLine( L"Fullscreen: F2" );
    pTxtHelper->DrawTextLine( L"Change device: F3" );
    pTxtHelper->DrawTextLine( L"Toggle warp: F4" );
    pTxtHelper->DrawTextLine( L"Toggle GUI: F10" );
    pTxtHelper->DrawTextLine( L"Toggle Demo Text: F11" );
    pTxtHelper->DrawTextLine( L"Rotate Camera: Left Mouse Button" );
    pTxtHelper->DrawTextLine( L"Rotate Light: Right Mouse Button" );
    pTxtHelper->DrawTextLine( L"Rotate Model: Middle Mouse Button" );
    pTxtHelper->DrawTextLine( L"Zoom Camera: Mouse Wheel Scroll" );
    pTxtHelper->DrawTextLine( L"Hide Help: F1" );								
    pTxtHelper->DrawTextLine( L"Quit: ESC" );	

    return;
}

