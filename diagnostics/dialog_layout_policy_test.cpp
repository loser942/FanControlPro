#include "../DialogLayoutPolicy.h"
#include <cassert>

int main()
{
    assert(DialogLayoutPolicy::SelectWindowHeight(false, 480, 760) == 480);
    assert(DialogLayoutPolicy::SelectWindowHeight(true, 480, 760) == 760);
    assert(!DialogLayoutPolicy::ShowAdvancedCommands(false));
    assert(DialogLayoutPolicy::ShowAdvancedCommands(true));
    return 0;
}
