// FanControlProDlg.cpp : 对话框实现
// Windows 11 现代化风格风扇控制程序

#include "stdafx.h"
#include "FanControlPro.h"
#include "FanControlProDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_SHOWTASK (WM_USER + 1)

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();
    enum { IDD = IDD_ABOUTBOX };
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD) {}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

CFanControlProDlg::CFanControlProDlg(CWnd* pParent)
    : CDialogEx(CFanControlProDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_bForceHideWindow = TRUE;
    m_hCoreThread = NULL;
    m_nLastCoreUpdateTime = -1;
    m_bWindowVisible = FALSE;
    m_bAdvancedMode = TRUE;
    m_nWindowSize[0] = 0;
    m_nWindowSize[1] = 0;
    m_nCheckThreadCount = 0;
    m_bLastVisible = FALSE;
    m_bTrayAdded = FALSE;
    m_nTrayLastUpdate = -1;
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 10; j++)
            m_nDutyEditCtlID[i][j] = 0;
}

CFanControlProDlg::~CFanControlProDlg() {}

void CFanControlProDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_STATUS, m_ctlStatus);
    DDX_Control(pDX, IDC_CHECK_TAKEOVER, m_ctlTakeOver);
    DDX_Control(pDX, IDC_CHECK_FORCE, m_ctlForcedCooling);
    DDX_Control(pDX, IDC_CHECK_LINEAR, m_ctlLinear);
    DDX_Control(pDX, IDC_EDIT_INTERVAL, m_ctlInterval);
    DDX_Control(pDX, IDC_EDIT_TRANSITION, m_ctlTransition);
    DDX_Control(pDX, IDC_EDIT_FORCE_TEMP, m_ctlForceTemp);
    DDX_Control(pDX, IDC_CHECK_AUTORUN, m_ctlAutorun);
    DDX_Control(pDX, IDC_EDIT_GPU_FREQUENCY, m_ctlFrequency);
    DDX_Control(pDX, IDC_CHECK_LOCK_GPU, m_ctlLockGpu);
    DDX_Control(pDX, IDC_COMBO_MODE, m_ctlMode);
    DDX_Control(pDX, IDC_SLIDER_CPU_FAN, m_ctlCpuFanSlider);
    DDX_Control(pDX, IDC_SLIDER_GPU_FAN, m_ctlGpuFanSlider);
    DDX_Control(pDX, IDC_EDIT_CPU_FAN, m_ctlCpuFanEdit);
    DDX_Control(pDX, IDC_EDIT_GPU_FAN, m_ctlGpuFanEdit);
    DDX_Control(pDX, IDC_CHECK_SILENT, m_ctlSilent);
    DDX_Control(pDX, IDC_CHECK_PERFORMANCE, m_ctlPerformance);
    DDX_Control(pDX, IDC_PROGRESS_CPU_TEMP, m_ctlCpuTempProgress);
    DDX_Control(pDX, IDC_PROGRESS_GPU_TEMP, m_ctlGpuTempProgress);
    DDX_Control(pDX, IDC_STATIC_CPU_TEMP, m_ctlCpuTempText);
    DDX_Control(pDX, IDC_STATIC_GPU_TEMP, m_ctlGpuTempText);
    DDX_Control(pDX, IDC_STATIC_CPU_RPM, m_ctlCpuRpmText);
    DDX_Control(pDX, IDC_STATIC_GPU_RPM, m_ctlGpuRpmText);
    DDX_Control(pDX, IDC_STATIC_CPU_USAGE, m_ctlCpuUsageText);
    DDX_Control(pDX, IDC_STATIC_GPU_USAGE, m_ctlGpuUsageText);
    DDX_Control(pDX, IDC_MAX_DUTY_SLIDER, m_ctlMaxDutySlider);
    DDX_Control(pDX, IDC_EDIT_MAX_DUTY, m_ctlMaxDutyEdit);
}

BEGIN_MESSAGE_MAP(CFanControlProDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_TIMER()
    ON_WM_HSCROLL()
    ON_BN_CLICKED(IDC_BUTTON_SAVE, &CFanControlProDlg::OnBnClickedButtonSave)
    ON_BN_CLICKED(IDC_BUTTON_RESET, &CFanControlProDlg::OnBnClickedButtonReset)
    ON_BN_CLICKED(IDC_BUTTON_LOAD, &CFanControlProDlg::OnBnClickedButtonLoad)
    ON_BN_CLICKED(IDC_BUTTON_ADVANCED, &CFanControlProDlg::OnBnClickedButtonAdvanced)
    ON_BN_CLICKED(IDC_CHECK_TAKEOVER, &CFanControlProDlg::OnBnClickedCheckTakeover)
    ON_BN_CLICKED(IDC_CHECK_FORCE, &CFanControlProDlg::OnBnClickedCheckForce)
    ON_BN_CLICKED(IDC_CHECK_LINEAR, &CFanControlProDlg::OnBnClickedCheckLinear)
    ON_BN_CLICKED(IDC_CHECK_AUTORUN, &CFanControlProDlg::OnBnClickedCheckAutorun)
    ON_BN_CLICKED(IDC_CHECK_LOCK_GPU, &CFanControlProDlg::OnBnClickedCheckLockGpu)
    ON_BN_CLICKED(IDC_CHECK_SILENT, &CFanControlProDlg::OnBnClickedSilent)
    ON_BN_CLICKED(IDC_CHECK_PERFORMANCE, &CFanControlProDlg::OnBnClickedPerformance)
    ON_CBN_SELCHANGE(IDC_COMBO_MODE, &CFanControlProDlg::OnCbnSelchangeComboMode)
    ON_MESSAGE(WM_SHOWTASK, &CFanControlProDlg::OnShowTask)
END_MESSAGE_MAP()

BOOL CFanControlProDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);
    CRect rect;
    this->GetWindowRect(rect);
    m_nWindowSize[0] = rect.Width();
    m_nWindowSize[1] = rect.Height();
    m_ctlMode.AddString("自动模式");
    m_ctlMode.AddString("手动模式");
    m_ctlMode.AddString("强冷模式");
    m_ctlMode.SetCurSel(0);
    m_ctlCpuFanSlider.SetRange(0, 100);
    m_ctlCpuFanSlider.SetPos(50);
    m_ctlGpuFanSlider.SetRange(0, 100);
    m_ctlGpuFanSlider.SetPos(50);
    m_ctlMaxDutySlider.SetRange(50, 100);
    m_ctlMaxDutySlider.SetPos(85);
    m_ctlMaxDutyEdit.SetWindowText("85");
    m_ctlCpuTempProgress.SetRange(0, 100);
    m_ctlGpuTempProgress.SetRange(0, 100);
    SetTray("FanControl Pro - 智能风扇控制");
    if (m_hCoreThread == NULL)
    {
        m_core.SetHWnd(this->m_hWnd);
        DWORD dwThreadID = 0;
        m_hCoreThread = CreateThread(NULL, NULL, CoreThread, this, NULL, &dwThreadID);
    }
    SetTimer(0, 100, NULL);
    m_ctlAutorun.SetCheck(SetAutorunReg(FALSE) || SetAutorunTask(FALSE));
    return TRUE;
}

void CFanControlProDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

void CFanControlProDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

HCURSOR CFanControlProDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

DWORD CFanControlProDlg::CoreThread(LPVOID lParam)
{
    CFanControlProDlg * pDlg = (CFanControlProDlg *)lParam;
    pDlg->m_core.Run();
    return 0;
}

void CFanControlProDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    if (m_bForceHideWindow)
    {
        lpwndpos->flags &= ~SWP_SHOWWINDOW;
    }
    CDialogEx::OnWindowPosChanging(lpwndpos);
}

void CFanControlProDlg::OnOK()
{
    if (!m_core.m_nExit)
        m_core.m_nExit = 1;
    if (m_core.m_nExit == 1)
    {
        int count = 0;
        while (m_core.m_nExit == 1 && count++ < 100)
        {
            Sleep(100);
        }
    }
    if (m_core.m_nExit)
    {
        KillTimer(0);
        SetTray(NULL);
        CDialogEx::OnOK();
    }
}

void CFanControlProDlg::OnCancel()
{
    if (m_bWindowVisible)
    {
        this->ShowWindow(SW_HIDE);
    }
    else
    {
        this->ShowWindow(SW_SHOW);
        SetForegroundWindow();
    }
}

void CFanControlProDlg::OnTimer(UINT_PTR nIDEvent)
{
    CDialogEx::OnTimer(nIDEvent);
    if (m_core.m_nExit == 2)
    {
        OnOK();
    }
    m_nCheckThreadCount++;
    if (m_nCheckThreadCount > 300)
    {
        KillTimer(0);
        m_core.m_nExit = 2;
        if (WaitForSingleObject(m_hCoreThread, 3000) == WAIT_TIMEOUT)
        {
            TerminateThread(m_hCoreThread, -1);
        }
        CloseHandle(m_hCoreThread);
        m_hCoreThread = NULL;
        m_core.ResetFan();
        MessageBox("检测到核心线程异常，请检查系统兼容性。");
        OnOK();
    }
    if (m_core.m_nInit != 1)
        return;
    m_bWindowVisible = IsWindowVisible();
    if (m_bWindowVisible && !m_bLastVisible)
    {
        m_core.m_bUpdateRPM = TRUE;
        UpdateGui(TRUE);
    }
    else if (!m_bWindowVisible && m_bLastVisible)
    {
        m_core.m_bUpdateRPM = FALSE;
    }
    m_bLastVisible = m_bWindowVisible;
    if (m_nLastCoreUpdateTime != m_core.m_nLastUpdateTime)
    {
        if (m_bWindowVisible)
            UpdateGui(FALSE);
        m_nCheckThreadCount = 0;
        m_nLastCoreUpdateTime = m_core.m_nLastUpdateTime;
    }
}

void CFanControlProDlg::UpdateGui(BOOL bFull)
{
    char str[256];
    sprintf_s(str, "CPU: %d°C", m_core.m_nCurTemp[0]);
    m_ctlCpuTempText.SetWindowText(str);
    m_ctlCpuTempProgress.SetPos(min(100, m_core.m_nCurTemp[0]));
    sprintf_s(str, "GPU: %d°C", m_core.m_nCurTemp[1]);
    m_ctlGpuTempText.SetWindowText(str);
    m_ctlGpuTempProgress.SetPos(min(100, m_core.m_nCurTemp[1]));
    if (m_core.m_nCurRPM[0] >= 0)
    {
        sprintf_s(str, "CPU RPM: %d", m_core.m_nCurRPM[0]);
        m_ctlCpuRpmText.SetWindowText(str);
    }
    else
    {
        m_ctlCpuRpmText.SetWindowText("CPU RPM: --");
    }
    if (m_core.m_nCurRPM[1] >= 0)
    {
        sprintf_s(str, "GPU RPM: %d", m_core.m_nCurRPM[1]);
        m_ctlGpuRpmText.SetWindowText(str);
    }
    else
    {
        m_ctlGpuRpmText.SetWindowText("GPU RPM: --");
    }
    sprintf_s(str, "GPU: %d%%", m_core.m_GpuInfo.m_nUsage);
    m_ctlGpuUsageText.SetWindowText(str);
    
    int nControlMode, nManualDuty0, nManualDuty1;
    m_core.LockConfig();
    nControlMode = m_core.m_config.ControlMode;
    nManualDuty0 = m_core.m_config.ManualDuty[0];
    nManualDuty1 = m_core.m_config.ManualDuty[1];
    m_core.UnlockConfig();
    if (nControlMode == 1)
    {
        m_ctlCpuFanSlider.SetPos(nManualDuty0);
        m_ctlGpuFanSlider.SetPos(nManualDuty1);
        sprintf_s(str, "%d%%", nManualDuty0);
        m_ctlCpuFanEdit.SetWindowText(str);
        sprintf_s(str, "%d%%", nManualDuty1);
        m_ctlGpuFanEdit.SetWindowText(str);
    }
    
    int fc = m_ctlForcedCooling.GetCheck();
    if (fc ^ m_core.m_bForcedCooling)
    {
        m_ctlForcedCooling.SetCheck(m_core.m_bForcedCooling);
    }
    if (!bFull)
        return;
    
    BOOL bTakeOver, bLinear, bLockGpu;
    int nUpdateInterval, nTransitionTemp, nForceTemp, nGpuFreq, nMaxDuty;
    m_core.LockConfig();
    bTakeOver       = m_core.m_config.TakeOver;
    bLinear         = m_core.m_config.Linear;
    bLockGpu        = m_core.m_config.LockGPUFrequency;
    nUpdateInterval = m_core.m_config.UpdateInterval;
    nTransitionTemp = m_core.m_config.TransitionTemp;
    nForceTemp      = m_core.m_config.ForceTemp;
    nGpuFreq        = m_core.m_config.GPUFrequency;
    nMaxDuty        = m_core.m_config.MaxDutyLimit;
    m_core.UnlockConfig();
    
    int to = m_ctlTakeOver.GetCheck();
    if (to ^ bTakeOver) m_ctlTakeOver.SetCheck(bTakeOver);
    int lc = m_ctlLinear.GetCheck();
    if (lc ^ bLinear) m_ctlLinear.SetCheck(bLinear);
    int lf = m_ctlLockGpu.GetCheck();
    if (lf ^ bLockGpu) m_ctlLockGpu.SetCheck(bLockGpu);
    
    sprintf_s(str, "%d", nUpdateInterval);
    m_ctlInterval.SetWindowTextA(str);
    sprintf_s(str, "%d", nTransitionTemp);
    m_ctlTransition.SetWindowTextA(str);
    sprintf_s(str, "%d", nForceTemp);
    m_ctlForceTemp.SetWindowTextA(str);
    sprintf_s(str, "%d", nGpuFreq);
    m_ctlFrequency.SetWindowTextA(str);
    sprintf_s(str, "%d", nMaxDuty);
    m_ctlMaxDutyEdit.SetWindowTextA(str);
    m_ctlMaxDutySlider.SetPos(nMaxDuty);
}

BOOL CFanControlProDlg::CheckAndSave()
{
    char str[256];
    m_ctlInterval.GetWindowTextA(str, 256);
    int nInterval = atoi(str);
    if (nInterval < 1 || nInterval > 5)
    {
        AfxMessageBox("更新间隔必须为 1-5 秒");
        m_ctlInterval.SetFocus();
        return FALSE;
    }
    m_ctlTransition.GetWindowTextA(str, 256);
    int nTransition = atoi(str);
    if (nTransition < 0 || nTransition > 10)
    {
        AfxMessageBox("过渡温度必须为 0-10");
        m_ctlTransition.SetFocus();
        return FALSE;
    }
    m_ctlForceTemp.GetWindowTextA(str, 256);
    int nForceTemp = atoi(str);
    if (nForceTemp < 40 || nForceTemp > 90)
    {
        AfxMessageBox("强冷温度必须为 40-90");
        m_ctlForceTemp.SetFocus();
        return FALSE;
    }
    m_ctlFrequency.GetWindowTextA(str, 256);
    int nFrequency = atoi(str);
    if (!CheckInputFrequency(nFrequency))
    {
        m_ctlFrequency.SetFocus();
        return FALSE;
    }
    if (nFrequency == 0)
        nFrequency = m_core.m_GpuInfo.m_nStandardFrequency;
    m_core.LockConfig();
    m_core.m_config.UpdateInterval = nInterval;
    m_core.m_config.TransitionTemp = nTransition;
    m_core.m_config.ForceTemp = nForceTemp;
    m_core.m_config.GPUFrequency = nFrequency;
    m_core.UnlockConfig();
    m_core.m_config.SaveConfig();
    return TRUE;
}

void CFanControlProDlg::OnBnClickedButtonSave()
{
    if (CheckAndSave()) UpdateGui(TRUE);
}

void CFanControlProDlg::OnBnClickedButtonReset()
{
    m_core.LockConfig();
    m_core.m_config.LoadDefault();
    m_core.UnlockConfig();
    m_core.m_config.SaveConfig();
    UpdateGui(TRUE);
}

void CFanControlProDlg::OnBnClickedButtonLoad()
{
    m_core.LockConfig();
    m_core.m_config.LoadConfig();
    m_core.UnlockConfig();
    UpdateGui(TRUE);
}

void CFanControlProDlg::SetTray(PCSTR string)
{
    NOTIFYICONDATA nid;
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = this->m_hWnd;
    nid.uID = IDR_MAINFRAME;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_SHOWTASK;
    nid.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
    if (string)
    {
        strcpy_s(nid.szTip, 128, string);
        if (!m_bTrayAdded)
        {
            Shell_NotifyIcon(NIM_ADD, &nid);
            m_bTrayAdded = TRUE;
        }
        else
        {
            Shell_NotifyIcon(NIM_MODIFY, &nid);
        }
    }
    else
        Shell_NotifyIcon(NIM_DELETE, &nid);
}

LRESULT CFanControlProDlg::OnShowTask(WPARAM wParam, LPARAM lParam)
{
    if (wParam != IDR_MAINFRAME) return 1;
    switch (lParam)
    {
    case WM_RBUTTONUP:
    {
        LPPOINT lpoint = new tagPOINT;
        ::GetCursorPos(lpoint);
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MFT_STRING, IDR_SHOW, m_bWindowVisible ? "隐藏" : "显示");
        menu.AppendMenu(MFT_STRING, IDR_FORCED, m_core.m_bForcedCooling ? "退出强冷" : "强冷模式");
        menu.AppendMenu(MFT_SEPARATOR);
        menu.AppendMenu(MFT_STRING, IDR_EXIT, "退出");
        SetForegroundWindow();
        int xx = TrackPopupMenu(menu, TPM_RETURNCMD, lpoint->x, lpoint->y, NULL, this->m_hWnd, NULL);
        if (xx == IDR_SHOW) OnCancel();
        else if (xx == IDR_FORCED) m_core.EnableForcedCooling(!m_core.m_bForcedCooling);
        else if (xx == IDR_EXIT) OnOK();
        HMENU hmenu = menu.Detach();
        menu.DestroyMenu();
        delete lpoint;
    }break;
    case WM_LBUTTONDBLCLK:
    {
        this->ShowWindow(SW_SHOW);
        SetForegroundWindow();
    }break;
    case WM_MOUSEMOVE:
    {
        if (m_nTrayLastUpdate != m_core.m_nLastUpdateTime)
        {
            char str[128];
            sprintf_s(str, "CPU: %d°C, %d%%\nGPU: %d°C, %d%%", 
                m_core.m_nCurTemp[0], m_core.m_nCurDuty[0], 
                m_core.m_nCurTemp[1], m_core.m_nCurDuty[1]);
            SetTray(str);
            m_nTrayLastUpdate = m_core.m_nLastUpdateTime;
        }
    }break;
    }
    return 0;
}

void CFanControlProDlg::OnBnClickedCheckTakeover()
{
    m_core.LockConfig();
    m_core.m_config.TakeOver = m_ctlTakeOver.GetCheck();
    m_core.UnlockConfig();
}

void CFanControlProDlg::OnBnClickedCheckForce()
{
    m_core.m_bForcedCooling = m_ctlForcedCooling.GetCheck();
    if (m_core.m_bForcedCooling)
    {
        m_ctlMode.SetCurSel(2);
        m_core.LockConfig();
        m_core.m_config.ControlMode = 2;
        m_core.UnlockConfig();
    }
}

void CFanControlProDlg::OnBnClickedCheckLinear()
{
    m_core.LockConfig();
    m_core.m_config.Linear = m_ctlLinear.GetCheck();
    m_core.UnlockConfig();
}

void CFanControlProDlg::SetAdvancedMode(BOOL bAdvanced)
{
    CRect rect;
    this->GetWindowRect(rect);
    if (bAdvanced)
    {
        MoveWindow(rect.left, rect.top, m_nWindowSize[0], m_nWindowSize[1], TRUE);
        GetDlgItem(IDC_BUTTON_ADVANCED)->SetWindowTextA("简洁模式");
    }
    else
    {
        MoveWindow(rect.left, rect.top, m_nWindowSize[0] * 335 / 582, m_nWindowSize[1] * 283 / 463, FALSE);
        GetDlgItem(IDC_BUTTON_ADVANCED)->SetWindowTextA("高级模式");
    }
    m_bAdvancedMode = !m_bAdvancedMode;
}

void CFanControlProDlg::OnBnClickedButtonAdvanced()
{
    SetAdvancedMode(!m_bAdvancedMode);
}

void CFanControlProDlg::OnBnClickedCheckAutorun()
{
    int val = m_ctlAutorun.GetCheck();
    if (val)
    {
        int rv = MessageBox("请选择开机自动启动方式：\r\n\r\n\"是\"=注册表方式（简单）\r\n\"否\"=任务计划程序（推荐）\r\n\"取消\"=放弃设置", 
            "设置开机自启", MB_YESNOCANCEL);
        if (IDYES == rv) { SetAutorunReg(TRUE, TRUE); SetAutorunReg(FALSE); }
        else if (IDNO == rv) { SetAutorunTask(TRUE, TRUE); SetAutorunTask(FALSE); }
        else { m_ctlAutorun.SetCheck(FALSE); }
    }
    else
    {
        SetAutorunReg(FALSE);
        SetAutorunTask(FALSE);
        m_ctlAutorun.SetCheck(SetAutorunReg(FALSE) || SetAutorunTask(FALSE));
    }
}

void CFanControlProDlg::OnBnClickedCheckLockGpu()
{
    m_core.LockConfig();
    m_core.m_config.LockGPUFrequency = m_ctlLockGpu.GetCheck();
    m_core.UnlockConfig();
}

void CFanControlProDlg::OnBnClickedSilent()
{
    m_core.ApplyPreset("Silent");
    UpdateGui(TRUE);
}

void CFanControlProDlg::OnBnClickedPerformance()
{
    m_core.ApplyPreset("Performance");
    UpdateGui(TRUE);
}

void CFanControlProDlg::OnCbnSelchangeComboMode()
{
    int sel = m_ctlMode.GetCurSel();
    m_core.SetControlMode(sel);
    BOOL bEnableSliders = (sel == 1);
    m_ctlCpuFanSlider.EnableWindow(bEnableSliders);
    m_ctlGpuFanSlider.EnableWindow(bEnableSliders);
    m_ctlCpuFanEdit.EnableWindow(bEnableSliders);
    m_ctlGpuFanEdit.EnableWindow(bEnableSliders);
}

void CFanControlProDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    if (pScrollBar == (CScrollBar*)&m_ctlCpuFanSlider)
    {
        int pos = m_ctlCpuFanSlider.GetPos();
        m_core.LockConfig();
        m_core.m_config.ManualDuty[0] = pos;
        m_core.UnlockConfig();
        char str[16];
        sprintf_s(str, "%d%%", pos);
        m_ctlCpuFanEdit.SetWindowText(str);
    }
    else if (pScrollBar == (CScrollBar*)&m_ctlGpuFanSlider)
    {
        int pos = m_ctlGpuFanSlider.GetPos();
        m_core.LockConfig();
        m_core.m_config.ManualDuty[1] = pos;
        m_core.UnlockConfig();
        char str[16];
        sprintf_s(str, "%d%%", pos);
        m_ctlGpuFanEdit.SetWindowText(str);
    }
    else if (pScrollBar == (CScrollBar*)&m_ctlMaxDutySlider)
    {
        int pos = m_ctlMaxDutySlider.GetPos();
        m_core.SetMaxDutyLimit(pos);
        char str[16];
        sprintf_s(str, "%d%%", pos);
        m_ctlMaxDutyEdit.SetWindowText(str);
    }
    CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CFanControlProDlg::SetAutorunReg(BOOL bWrite, BOOL bAutorun)
{
    HKEY hKey;
    if (RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey) != ERROR_SUCCESS)
        return FALSE;
    PCSTR strProduct = "FanControlPro";
    if (bWrite)
    {
        if (bAutorun)
        {
            CString strPath = GetExePath() + "\\FanControlPro.exe";
            unsigned long nSize = strPath.GetLength();
            if (RegSetValueEx(hKey, strProduct, 0, REG_SZ,
                (unsigned char *)strPath.GetBuffer(nSize), nSize) != ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                return FALSE;
            }
        }
        else { RegDeleteValue(hKey, strProduct); }
        RegCloseKey(hKey);
    }
    else
    {
        unsigned long lSize = 0;
        if (RegQueryValueEx(hKey, strProduct, NULL, NULL, NULL, &lSize) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }
        RegCloseKey(hKey);
        return lSize > 0 ? TRUE : FALSE;
    }
    return TRUE;
}

BOOL CFanControlProDlg::SetAutorunTask(BOOL bWrite, BOOL bAutorun)
{
    CString strTaskName = "FanControlPro";
    CString strPath = GetExePath() + "\\FanControlPro.exe";
    CString strcmd;
    CString strXmlPath = GetExePath() + "\\task.xml";
    if (bWrite)
    {
        if (bAutorun)
        {
            if (!CreateTaskXml(strXmlPath, strPath)) return FALSE;
            strcmd.Format("SCHTASKS /Create /F /XML \"%s\" /TN \"%s\"", strXmlPath, strTaskName);
        }
        else { strcmd = "SCHTASKS /Delete /F /TN \"" + strTaskName + "\""; }
    }
    else { strcmd = "SCHTASKS /Query /TN \"" + strTaskName + "\""; }
    CString rs = ExecuteCmd(strcmd);
    if (bWrite && bAutorun) remove(strXmlPath);
    if (rs.Find("拒绝访问") >= 0) return FALSE;
    if (bWrite && bAutorun && rs.Find("成功") >= 0) return TRUE;
    if (!bWrite && rs.Find(strTaskName) >= 0) return TRUE;
    if (bWrite && !bAutorun) return TRUE;
    return FALSE;
}

CString CFanControlProDlg::ExecuteCmd(CString str)
{
    SECURITY_ATTRIBUTES sa;
    HANDLE hRead, hWrite;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return "[执行失败]";
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.hStdError = hWrite;
    si.hStdOutput = hWrite;
    si.wShowWindow = SW_HIDE;
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    char cmdline[1024];
    strcpy_s(cmdline, 1024, str);
    if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
    {
        CloseHandle(hWrite);
        CloseHandle(hRead);
        return "[执行失败]";
    }
    CloseHandle(hWrite);
    char buffer[4096] = "";
    CString output;
    DWORD byteRead;
    int i = 0;
    while (true)
    {
        Sleep(100);
        if (ReadFile(hRead, buffer, 4095, &byteRead, NULL) == NULL) break;
        if (byteRead) output += buffer;
        if (i++ >= 50) break;
    }
    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return output;
}

BOOL CFanControlProDlg::CreateTaskXml(PCSTR strXmlPath, PCSTR strTargetPath)
{
    PCSTR XmlStr = 
"<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n"
"<Task version=\"1.2\" xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\r\n"
"  <RegistrationInfo>\r\n"
"    <Author>FanControlPro</Author>\r\n"
"  </RegistrationInfo>\r\n"
"  <Triggers>\r\n"
"    <LogonTrigger>\r\n"
"      <Enabled>true</Enabled>\r\n"
"    </LogonTrigger>\r\n"
"  </Triggers>\r\n"
"  <Principals>\r\n"
"    <Principal id=\"Author\">\r\n"
"      <GroupId>S-1-5-32-545</GroupId>\r\n"
"      <RunLevel>HighestAvailable</RunLevel>\r\n"
"    </Principal>\r\n"
"  </Principals>\r\n"
"  <Settings>\r\n"
"    <MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>\r\n"
"    <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>\r\n"
"    <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>\r\n"
"    <AllowHardTerminate>false</AllowHardTerminate>\r\n"
"    <StartWhenAvailable>false</StartWhenAvailable>\r\n"
"    <RunOnlyIfNetworkAvailable>false</RunOnlyIfNetworkAvailable>\r\n"
"    <IdleSettings>\r\n"
"      <StopOnIdleEnd>false</StopOnIdleEnd>\r\n"
"      <RestartOnIdle>false</RestartOnIdle>\r\n"
"    </IdleSettings>\r\n"
"    <AllowStartOnDemand>true</AllowStartOnDemand>\r\n"
"    <Enabled>true</Enabled>\r\n"
"    <Hidden>true</Hidden>\r\n"
"    <RunOnlyIfIdle>false</RunOnlyIfIdle>\r\n"
"    <WakeToRun>false</WakeToRun>\r\n"
"    <ExecutionTimeLimit>PT0S</ExecutionTimeLimit>\r\n"
"    <Priority>7</Priority>\r\n"
"  </Settings>\r\n"
"  <Actions Context=\"Author\">\r\n"
"    <Exec>\r\n"
"      <Command>%s</Command>\r\n"
"    </Exec>\r\n"
"  </Actions>\r\n"
"</Task>\r\n";
    char str[10240];
    sprintf_s(str, 10240, XmlStr, strTargetPath);
    FILE *fp = fopen(strXmlPath, "wt");
    if (!fp) return FALSE;
    fwrite(str, strlen(str), 1, fp);
    fclose(fp);
    return TRUE;
}

CString CFanControlProDlg::GetExePath()
{
    return ::GetExePath();
}

BOOL CFanControlProDlg::CheckInputFrequency(int nFrequency)
{
    if (nFrequency < 0 || nFrequency > m_core.m_GpuInfo.m_nMaxFrequency)
    {
        char str[256];
        sprintf_s(str, "GPU 频率必须为 0-%d", m_core.m_GpuInfo.m_nMaxFrequency);
        AfxMessageBox(str);
        return FALSE;
    }
    return TRUE;
}