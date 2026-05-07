#pragma once

#ifndef __AFXWIN_H__
    #error "在包含此文件之前包含 'stdafx.h'"
#endif

#include "resource.h"

class CFanControlProApp : public CWinApp
{
public:
    CFanControlProApp();

// 重写
public:
    virtual BOOL InitInstance();

// 实现
    DECLARE_MESSAGE_MAP()
};

extern CFanControlProApp theApp;