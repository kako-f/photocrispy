#include "FileBrowser.h"

#include <cassert>

int main()
{
    FileBrowser browser;
    assert(!browser.Refresh("this-directory-must-not-exist"));
    return 0;
}
