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

    static bool ShowMonitoringControls(bool advanced)
    {
        return !advanced;
    }

    static bool ControlsFit(int dialogHeight, int controlTop, int controlHeight)
    {
        return controlTop >= 0 && controlHeight >= 0 && controlTop + controlHeight <= dialogHeight;
    }
};
