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


//--------------------------------------------------------------------------------------
// Simple container for a very light weight scene description file loader
//--------------------------------------------------------------------------------------
class SceneDescription
{
public:
	SceneDescription();
	~SceneDescription();

	void LoadFromFile( wchar_t* filename );

	unsigned int GetNumModels() const;

	// GetModelPath returns full path
	const wchar_t* GetModelPath( unsigned int model ) const;

	// GetModelFilename returns file name only, no path nor extension
	const wchar_t* GetModelFilename( unsigned int model ) const;

	// GetAnimationPath returns full path of animation
	const wchar_t* GetAnimationPath( unsigned int model ) const;
	const wchar_t* GetParams( unsigned int model ) const;

private:
	class Pimpl;
	Pimpl* m_PrivateImplementation;
};

