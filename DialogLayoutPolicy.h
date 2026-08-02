#pragma once

class DialogLayoutPolicy
{
public:
    static int SelectWindowHeight(bool advanced, int compactHeight, int expandedHeight)
    {
        return advanced ? expandedHeight : compactHeight;
    }

    static bool ShowAdvancedCommands(bool advanced)
    {
        return advanced;
    }
};
