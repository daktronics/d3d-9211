// Copyright (c) 2018 Daktronics. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#pragma once

#include <d3dcompiler.h>
#include <directxmath.h>

#include <memory>
#include <string>

namespace d3d {

	//
	// compiler for shader source code
	//
	class ICompiler
	{
	public:
		ICompiler() {}
		virtual ~ICompiler() {}

		virtual std::shared_ptr<ID3DBlob> compile(
			std::string const& source_code,
			std::string const& entry_point,
			std::string const& model) = 0;

	private:
		ICompiler(ICompiler const&) = delete;
		ICompiler& operator=(ICompiler const&) = delete;
	};

	std::shared_ptr<ICompiler> create_compiler();

}