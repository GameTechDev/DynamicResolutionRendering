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

