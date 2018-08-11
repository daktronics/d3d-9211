#include "console.h"
#include "util.h"
#include "assets.h"

using namespace std;

namespace {

	class Console : public IConsole
	{
	private:

	public:
		Console() {
		}


	};
}

shared_ptr<IConsole> create_console(shared_ptr<IFontAtlas> const&)
{
	return make_shared<Console>();
}
