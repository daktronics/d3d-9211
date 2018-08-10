#include "d3d.h"
#include "util.h"

using namespace std;

namespace {

	//
	// wrap D3DCompile in our ICompiler interface
	//
	// note: used by both Direct3D 9 and 11
	//
	class Compiler : public d3d::ICompiler
	{
	private:
		HMODULE const library_;

	public:
		Compiler(HMODULE library)
			: library_(library) {
		}

		~Compiler() {
			FreeLibrary(library_);
		}

		shared_ptr<ID3DBlob> compile(
			string const& source_code,
			string const& entry_point,
			string const& model)
		{
			if (!library_) {
				return nullptr;
			}

			typedef HRESULT(WINAPI* PFN_D3DCOMPILE)(
				LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*,
				ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);

			auto const fnc_compile = reinterpret_cast<PFN_D3DCOMPILE>(
				GetProcAddress(library_, "D3DCompile"));
			if (!fnc_compile) {
				return nullptr;
			}

			DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(NDEBUG)
			//flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
			//flags |= D3DCOMPILE_AVOID_FLOW_CONTROL;
#else
			flags |= D3DCOMPILE_DEBUG;
			flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

			ID3DBlob* blob = nullptr;
			ID3DBlob* blob_err = nullptr;

			auto const psrc = source_code.c_str();
			auto const len = source_code.size() + 1;

			auto const hr = fnc_compile(
				psrc, len, nullptr, nullptr, nullptr,
				entry_point.c_str(),
				model.c_str(),
				flags,
				0,
				&blob,
				&blob_err);

			if (FAILED(hr))
			{
				if (blob_err)
				{
					log_message("%s\n", (char*)blob_err->GetBufferPointer());
					blob_err->Release();
				}
				return nullptr;
			}

			if (blob_err) {
				blob_err->Release();
			}

			return to_com_ptr<>(blob);
		}
	};
}

namespace d3d {

	shared_ptr<ICompiler> create_compiler()
	{
		auto const lib = LoadLibrary(L"d3dcompiler_47.dll");
		if (lib) {
			return make_shared<Compiler>(lib);
		}
		return nullptr;
	}
}