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
#include "SceneDescription.h"

#include <vector>
#include <string>
#include <fstream>

//--------------------------------------------------------------------------------------
// Private data to prevent include of std into other compilation units
//--------------------------------------------------------------------------------------
class SceneDescription::Pimpl
{
public:
	std::vector<std::wstring> m_ModelPaths;
	std::vector<std::wstring> m_ModelFilenames;
	std::vector<std::wstring> m_AnimationPaths;
	std::vector<std::wstring> m_Params;
};


//--------------------------------------------------------------------------------------
// Ctor
//--------------------------------------------------------------------------------------
SceneDescription::SceneDescription()
	: m_PrivateImplementation( NULL )
{
}

//--------------------------------------------------------------------------------------
// Dtor
//--------------------------------------------------------------------------------------
SceneDescription::~SceneDescription()
{
	delete m_PrivateImplementation;
}

//--------------------------------------------------------------------------------------
// Load data from file
//
// This function has very little in the way of error checking and debug output as it
// is not intended to be a robust production scene description loader
//--------------------------------------------------------------------------------------
void SceneDescription::LoadFromFile( wchar_t* filename )
{
	m_PrivateImplementation = new Pimpl;
	std::wifstream file;
	file.open( filename );

	if( m_PrivateImplementation && !file.fail() )
	{
		while( !file.eof() )
		{
			std::wstring modelPath;
			file >> modelPath;
			if( !file.eof() )
			{
				m_PrivateImplementation->m_ModelPaths.push_back( modelPath );
				//strip path to filename
				std::wstring filename = modelPath.substr( modelPath.find_last_of( L"\\" ) + 1 );
				filename = filename.substr( filename.find_last_of( L"/" ) + 1 );
				filename = filename.substr( 0, filename.find_last_of( L"." ) );
				m_PrivateImplementation->m_ModelFilenames.push_back( filename );

				//we always get the animation filename
				std::wstring animationPath;
				file >> animationPath;
				if( 0 == animationPath.compare( L"NO_ANIM" ) )
				{
					animationPath.clear();
				}
				//always push back even if empty
				m_PrivateImplementation->m_AnimationPaths.push_back( animationPath );

				//we always get the param string
				std::wstring params;
				file >> params;
				//always push back even if empty
				m_PrivateImplementation->m_Params.push_back( params );

			}

		}
	}
}

//--------------------------------------------------------------------------------------
// Dtor
//--------------------------------------------------------------------------------------
unsigned int SceneDescription::GetNumModels() const
{
	if( m_PrivateImplementation )
	{
		return static_cast<unsigned int>( m_PrivateImplementation->m_ModelPaths.size() );
	}
	else
	{
		return 0;
	}
}

//--------------------------------------------------------------------------------------
// Return the path of the model if there is a valid one, else NULL
//--------------------------------------------------------------------------------------
const wchar_t* SceneDescription::GetModelPath( unsigned int model ) const
{
	if( m_PrivateImplementation && model < m_PrivateImplementation->m_ModelPaths.size() )
	{
		return m_PrivateImplementation->m_ModelPaths[ model ].c_str();
	}
	else
	{
		return NULL;
	}
}

//--------------------------------------------------------------------------------------
// Return the unadorned filename of the model if there is a valid one, else NULL
// This is intended for use in displaying the model name (warning may be degenerate)
//--------------------------------------------------------------------------------------
const wchar_t* SceneDescription::GetModelFilename( unsigned int model ) const
{
	if( m_PrivateImplementation && model < m_PrivateImplementation->m_ModelFilenames.size() )
	{
		return m_PrivateImplementation->m_ModelFilenames[ model ].c_str();
	}
	else
	{
		return NULL;
	}
}


//--------------------------------------------------------------------------------------
// Return the path of the animation if there is a valid one, else NULL
//--------------------------------------------------------------------------------------
const wchar_t* SceneDescription::GetAnimationPath( unsigned int model ) const
{
	if( m_PrivateImplementation && model < m_PrivateImplementation->m_AnimationPaths.size() && !m_PrivateImplementation->m_AnimationPaths[ model ].empty() )
	{
		return m_PrivateImplementation->m_AnimationPaths[ model ].c_str();
	}
	else
	{
		return NULL;
	}
}

//--------------------------------------------------------------------------------------
// Return the parameter line content, or NULL if none
//--------------------------------------------------------------------------------------
const wchar_t* SceneDescription::GetParams( unsigned int model ) const
{
	if( m_PrivateImplementation && model < m_PrivateImplementation->m_Params.size() && !m_PrivateImplementation->m_Params[ model ].empty() )
	{
		return m_PrivateImplementation->m_Params[ model ].c_str();
	}
	else
	{
		return NULL;
	}
}
