// FanControlProDlg.cpp : 对话框实现
// Windows 11 现代化风格风扇控制程序

#include "stdafx.h"
#include "FanControlPro.h"
#include "FanControlProDlg.h"
#include "AutorunCommandPolicy.h"
#include "DialogLayoutPolicy.h"
#include "afxdialogex.h"
#include <strsafe.h>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_SHOWTASK (WM_USER + 1)
#define WM_DEFERRED_STARTUP (WM_APP + 1)

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
    // 首次启动应显示主窗口；仅在用户主动最小化后通过托盘恢复。
    m_bForceHideWindow = FALSE;
m_hCoreThread = NULL;
     m_hSingleInstanceMutex = NULL;
     m_nLastCoreUpdateTime = -1;
    m_bWindowVisible = FALSE;
    m_bAdvancedMode = FALSE;
    m_nWindowSize[0] = 0;
    m_nWindowSize[1] = 0;
    m_nCompactWindowHeight = 0;
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
    DDX_Control(pDX, IDC_STATIC_STARTUP_STATUS, m_ctlStartupStatus);
    DDX_Control(pDX, IDC_STATIC_CONTROL_STATUS, m_ctlControlStatus);
    DDX_Control(pDX, IDC_STATIC_WARNING_STATUS, m_ctlWarningStatus);
    DDX_Control(pDX, IDC_CHECK_DESKTOP_NOTIFICATIONS, m_ctlDesktopNotifications);
    DDX_Control(pDX, IDC_EDIT_WARNING_TEMP, m_ctlWarningTemp);
    DDX_Control(pDX, IDC_EDIT_NOTIFICATION_COOLDOWN, m_ctlNotificationCooldown);
}

BEGIN_MESSAGE_MAP(CFanControlProDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_SIZE()
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
    ON_MESSAGE(WM_DEFERRED_STARTUP, &CFanControlProDlg::OnDeferredStartup)
    ON_MESSAGE(WM_CORE_INIT_RESULT, &CFanControlProDlg::OnCoreInitResult)
END_MESSAGE_MAP()

BOOL CFanControlProDlg::OnInitDialog()
 {
     CDialogEx::OnInitDialog();
     WriteDiagnosticLog("Dialog initialization started");

     // ── 单实例互斥 ──
     HANDLE hMutex = CreateMutexW(NULL, TRUE, L"FanControlPro_SingleInstance");
     if (GetLastError() == ERROR_ALREADY_EXISTS)
     {
         WriteDiagnosticLog("Single-instance mutex already exists");
         CloseHandle(hMutex);
        AfxMessageBox(L"Another FanControlPro instance is already running.");
         ExitProcess(0);
         return FALSE;
     }
      m_hSingleInstanceMutex = hMutex;
      WriteDiagnosticLog("Single-instance mutex acquired");

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
     m_nCompactWindowHeight = rect.Height();
     m_ctlMode.AddString(L"自动模式");
     m_ctlMode.AddString(L"手动模式");
     m_ctlMode.AddString(L"强冷模式");
     m_ctlMode.SetCurSel(0);
     m_ctlCpuFanSlider.SetRange(0, 100);
     m_ctlCpuFanSlider.SetPos(50);
     m_ctlGpuFanSlider.SetRange(0, 100);
     m_ctlGpuFanSlider.SetPos(50);
     m_ctlMaxDutySlider.SetRange(50, 100);
     m_ctlMaxDutySlider.SetPos(85);
     m_ctlMaxDutyEdit.SetWindowText(L"85");
     m_ctlCpuTempProgress.SetRange(0, 100);
     m_ctlGpuTempProgress.SetRange(0, 100);
     WriteDiagnosticLog("UI controls ready");
     SetStartupStatus(L"正在初始化监控，BIOS 风扇控制保持启用");
     SetTakeoverControlsEnabled(FALSE);
     SetAdvancedMode(FALSE);
     WriteDiagnosticLog("UI visible startup posted");
     PostMessage(WM_DEFERRED_STARTUP);

     // ── 窗口初始化完成后允许正常显示 ──
     m_bForceHideWindow = FALSE;

     return TRUE;
 }

LRESULT CFanControlProDlg::OnDeferredStartup(WPARAM, LPARAM)
{
    WriteDiagnosticLog("Deferred startup started");
    if (m_hCoreThread == NULL)
    {
        m_core.SetHWnd(this->m_hWnd);
        DWORD threadId = 0;
        m_hCoreThread = CreateThread(NULL, NULL, CoreThread, this, NULL, &threadId);
        if (m_hCoreThread == NULL)
        {
            char line[128] = {};
            sprintf_s(line, "Core thread creation failed. LastError=%lu", GetLastError());
            WriteDiagnosticLog(line);
            SetStartupStatus(L"无法启动监控线程，BIOS 风扇控制保持启用");
            return 0;
        }
        WriteDiagnosticLog("Core thread created");
    }

    SetTimer(0, 100, NULL);
    WriteDiagnosticLog("Optional tray setup started");
    SetTray(L"FanControl Pro - 智能风扇控制");
    WriteDiagnosticLog("Optional autorun status check started");
    m_ctlAutorun.SetCheck(QueryAutorunReg() || QueryAutorunTask());
    return 0;
}

LRESULT CFanControlProDlg::OnCoreInitResult(WPARAM wParam, LPARAM)
{
    if (wParam != 0)
    {
        SetStartupStatus(L"监控已就绪；请确认读数后再启用接管");
        SetTakeoverControlsEnabled(TRUE);
        WriteDiagnosticLog("UI core initialization succeeded");
    }
    else
    {
        CStringW status;
        status.Format(L"监控初始化失败（错误 %lu），BIOS 风扇控制保持启用", m_core.GetInitError());
        SetStartupStatus(status);
        SetTakeoverControlsEnabled(FALSE);
        WriteDiagnosticLog("UI core initialization failed");
    }
    return 0;
}

void CFanControlProDlg::SetStartupStatus(PCWSTR status)
{
    if (m_ctlStartupStatus.GetSafeHwnd())
        m_ctlStartupStatus.SetWindowTextW(status);
}

void CFanControlProDlg::SetTakeoverControlsEnabled(BOOL enabled)
{
    m_ctlTakeOver.EnableWindow(enabled);
    m_ctlForcedCooling.EnableWindow(enabled);
    m_ctlMode.EnableWindow(enabled);
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

DWORD WINAPI CFanControlProDlg::CoreThread(LPVOID lParam)
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
     {
         m_core.m_nExit = 1;
         m_core.SignalExit();
     }
     if (m_hCoreThread)
     {
         DWORD waitResult = WaitForSingleObject(m_hCoreThread, 5000);
         if (waitResult == WAIT_TIMEOUT)
         {
             // 线程卡死：ExitProcess 原子退出
             m_core.m_hWnd = NULL;
             KillTimer(0);
             SetTray(NULL);
             if (m_hSingleInstanceMutex) { CloseHandle(m_hSingleInstanceMutex); m_hSingleInstanceMutex = NULL; }
             ExitProcess(1);
         }
         CloseHandle(m_hCoreThread);
         m_hCoreThread = NULL;
     }
if (m_core.m_nExit)
     {
         KillTimer(0);
         SetTray(NULL);
        if (m_hSingleInstanceMutex) { CloseHandle(m_hSingleInstanceMutex); m_hSingleInstanceMutex = NULL; }
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
         return;
     }
if (m_core.m_nInit != 1)
         return;
     m_nCheckThreadCount++;
     if (m_nCheckThreadCount > 300)
     {
         KillTimer(0);
         m_core.m_nExit = 2;
         m_core.SignalExit();
         
         if (WaitForSingleObject(m_hCoreThread, 5000) == WAIT_TIMEOUT)
         {
             // 线程卡死：ExitProcess 原子退出，避免窗口销毁后线程仍访问 this
             MessageBox(L"检测到核心线程异常，程序将退出。");
             ExitProcess(1);
         }
         else
         {
CloseHandle(m_hCoreThread);
              m_hCoreThread = NULL;
          }
          OnOK();
          return;
      }
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
    CStringW text;
    text.Format(L"CPU: %d°C", m_core.m_nCurTemp[0].load());
    m_ctlCpuTempText.SetWindowTextW(text);
    m_ctlCpuTempProgress.SetPos(min(100, m_core.m_nCurTemp[0].load()));
    text.Format(L"GPU: %d°C", m_core.m_nCurTemp[1].load());
    m_ctlGpuTempText.SetWindowTextW(text);
    m_ctlGpuTempProgress.SetPos(min(100, m_core.m_nCurTemp[1].load()));
    if (m_core.m_nCurRPM[0].load() >= 0)
    {
        text.Format(L"CPU RPM: %d", m_core.m_nCurRPM[0].load());
        m_ctlCpuRpmText.SetWindowTextW(text);
    }
    else
    {
        m_ctlCpuRpmText.SetWindowTextW(L"CPU RPM: --");
    }
    if (m_core.m_nCurRPM[1].load() >= 0)
    {
        text.Format(L"GPU RPM: %d", m_core.m_nCurRPM[1].load());
        m_ctlGpuRpmText.SetWindowTextW(text);
    }
    else
    {
        m_ctlGpuRpmText.SetWindowTextW(L"GPU RPM: --");
    }
// GPU 使用率
     text.Format(L"GPU 使用率: %d%%", m_core.m_GpuInfo.m_nUsage);
     m_ctlGpuUsageText.SetWindowTextW(text);
     
     // CPU 使用率（占位，MFC 无内建 CPU 使用率 API，显示 CPU 温度代替）
     text.Format(L"CPU 使用率: %d%%", m_core.m_nCurTemp[0].load());
     m_ctlCpuUsageText.SetWindowTextW(text);
    int nControlMode, nManualDuty0, nManualDuty1, warningTemp, cooldownMinutes;
    BOOL desktopNotifications;
    m_core.LockConfig();
    nControlMode = m_core.m_config.ControlMode;
    nManualDuty0 = m_core.m_config.ManualDuty[0];
    nManualDuty1 = m_core.m_config.ManualDuty[1];
    warningTemp = m_core.m_config.WarningTemp;
    cooldownMinutes = m_core.m_config.NotificationCooldownMinutes;
    desktopNotifications = m_core.m_config.DesktopNotifications;
    m_core.UnlockConfig();
    if (nControlMode == 1)
    {
        m_ctlCpuFanSlider.SetPos(nManualDuty0);
        m_ctlGpuFanSlider.SetPos(nManualDuty1);
        text.Format(L"%d%%", nManualDuty0);
        m_ctlCpuFanEdit.SetWindowTextW(text);
        text.Format(L"%d%%", nManualDuty1);
        m_ctlGpuFanEdit.SetWindowTextW(text);
    }
    
    int fc = m_ctlForcedCooling.GetCheck();
    if (fc ^ m_core.m_bForcedCooling.load())
    {
        m_ctlForcedCooling.SetCheck(m_core.m_bForcedCooling.load());
    }
    UpdateControlStatus();
    if (!bFull)
    {
        UpdateWarningStatus();
        return;
    }
    
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
    
    text.Format(L"%d", nUpdateInterval);
    m_ctlInterval.SetWindowTextW(text);
    text.Format(L"%d", nTransitionTemp);
    m_ctlTransition.SetWindowTextW(text);
    text.Format(L"%d", nForceTemp);
    m_ctlForceTemp.SetWindowTextW(text);
    text.Format(L"%d", nGpuFreq);
    m_ctlFrequency.SetWindowTextW(text);
    text.Format(L"%d", nMaxDuty);
    m_ctlMaxDutyEdit.SetWindowTextW(text);
    text.Format(L"%d", warningTemp);
    m_ctlWarningTemp.SetWindowTextW(text);
    text.Format(L"%d", cooldownMinutes);
    m_ctlNotificationCooldown.SetWindowTextW(text);
    m_ctlDesktopNotifications.SetCheck(desktopNotifications);
    m_ctlMaxDutySlider.SetPos(nMaxDuty);
    UpdateWarningStatus();
}

BOOL CFanControlProDlg::CheckAndSave()
{
    CStringW text;
    m_ctlInterval.GetWindowTextW(text);
    int nInterval = _wtoi(text);
    if (nInterval < 1 || nInterval > 5)
    {
        AfxMessageBox(L"更新间隔必须为 1-5 秒");
        m_ctlInterval.SetFocus();
        return FALSE;
    }
    m_ctlTransition.GetWindowTextW(text);
    int nTransition = _wtoi(text);
    if (nTransition < 0 || nTransition > 10)
    {
        AfxMessageBox(L"过渡温度必须为 0-10");
        m_ctlTransition.SetFocus();
        return FALSE;
    }
    m_ctlForceTemp.GetWindowTextW(text);
    int nForceTemp = _wtoi(text);
    if (nForceTemp < 60 || nForceTemp > 95)
    {
        AfxMessageBox(L"强冷温度必须为 60-95");
        m_ctlForceTemp.SetFocus();
        return FALSE;
    }
    m_ctlFrequency.GetWindowTextW(text);
    int nFrequency = _wtoi(text);
    if (!CheckInputFrequency(nFrequency))
    {
        m_ctlFrequency.SetFocus();
        return FALSE;
    }
    if (nFrequency == 0)
        nFrequency = m_core.m_GpuInfo.m_nStandardFrequency;
    m_ctlWarningTemp.GetWindowTextW(text);
    const int warningTemp = _wtoi(text);
    if (warningTemp < 60 || warningTemp > 100)
    {
        AfxMessageBox(L"告警温度必须为 60-100°C");
        m_ctlWarningTemp.SetFocus();
        return FALSE;
    }
    m_ctlNotificationCooldown.GetWindowTextW(text);
    const int cooldownMinutes = _wtoi(text);
    if (cooldownMinutes < 1 || cooldownMinutes > 60)
    {
        AfxMessageBox(L"最短通知间隔必须为 1-60 分钟");
        m_ctlNotificationCooldown.SetFocus();
        return FALSE;
    }
    m_core.LockConfig();
    m_core.m_config.UpdateInterval = nInterval;
    m_core.m_config.TransitionTemp = nTransition;
    m_core.m_config.ForceTemp = nForceTemp;
    m_core.m_config.GPUFrequency = nFrequency;
    m_core.UnlockConfig();
    m_core.SaveConfigSnapshot();
    m_core.SetWarningSettings(m_ctlDesktopNotifications.GetCheck(), warningTemp, cooldownMinutes);
    return TRUE;
}

void CFanControlProDlg::UpdateWarningStatus()
{
    if (!m_ctlWarningStatus.GetSafeHwnd())
        return;

    m_ctlWarningStatus.SetWindowTextW(
        m_core.IsTemperatureWarning() ? L"温度告警：请检查散热状态" : L"温度正常");
}

void CFanControlProDlg::UpdateControlStatus()
{
    if (!m_ctlControlStatus.GetSafeHwnd())
        return;

    CStringW text;
    switch (m_core.GetControlVerification())
    {
    case ControlVerification::BiosControl:
        text = L"BIOS 控制中";
        break;
    case ControlVerification::RequestingTakeover:
        text = L"正在请求接管";
        break;
    case ControlVerification::Active:
        text.Format(
            L"接管已生效：CPU 目标 %d%% / EC %d%%\r\nGPU 目标 %d%% / EC %d%%",
            m_core.GetTargetDuty(0), m_core.GetReadbackDuty(0),
            m_core.GetTargetDuty(1), m_core.GetReadbackDuty(1));
        break;
    case ControlVerification::Fault:
    default:
        text = L"接管状态异常";
        break;
    }
    m_ctlControlStatus.SetWindowTextW(text);
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
    m_core.SaveConfigSnapshot();
    UpdateGui(TRUE);
}

void CFanControlProDlg::OnBnClickedButtonLoad()
{
    m_core.LockConfig();
    m_core.m_config.LoadConfig();
    m_core.UnlockConfig();
    UpdateGui(TRUE);
}

void CFanControlProDlg::SetTray(PCWSTR string)
{
    NOTIFYICONDATAW nid = { sizeof(NOTIFYICONDATAW) };
    nid.hWnd = this->m_hWnd;
    nid.uID = IDR_MAINFRAME;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_SHOWTASK;
    nid.hIcon = LoadIconW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDR_MAINFRAME));
    if (string)
    {
        StringCchCopyW(nid.szTip, _countof(nid.szTip), string);
        if (!m_bTrayAdded)
        {
            Shell_NotifyIconW(NIM_ADD, &nid);
            m_bTrayAdded = TRUE;
        }
        else
        {
            Shell_NotifyIconW(NIM_MODIFY, &nid);
        }
    }
    else
        Shell_NotifyIconW(NIM_DELETE, &nid);
}

LRESULT CFanControlProDlg::OnShowTask(WPARAM wParam, LPARAM lParam)
{
    if (wParam != IDR_MAINFRAME) return 1;
    switch (lParam)
    {
    case WM_RBUTTONUP:
    {
        POINT point = {};
        ::GetCursorPos(&point);
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenuW(MFT_STRING, IDR_SHOW, m_bWindowVisible ? L"隐藏" : L"显示");
        menu.AppendMenuW(MFT_STRING, IDR_FORCED, m_core.m_bForcedCooling.load() ? L"退出强冷" : L"强冷模式");
        menu.AppendMenu(MFT_SEPARATOR);
        menu.AppendMenuW(MFT_STRING, IDR_EXIT, L"退出");
        SetForegroundWindow();
        int xx = TrackPopupMenu(menu, TPM_RETURNCMD, point.x, point.y, NULL, this->m_hWnd, NULL);
        if (xx == IDR_SHOW) OnCancel();
        else if (xx == IDR_FORCED)
        {
            if (!CanEnableForcedCooling(m_core.GetStartupState(), m_core.m_userTakeoverAuthorized.load()))
            {
                m_ctlForcedCooling.SetCheck(FALSE);
                AfxMessageBox(L"请先启用风扇接管");
            }
            else
                m_core.EnableForcedCooling(!m_core.m_bForcedCooling.load());
        }
        else if (xx == IDR_EXIT) OnOK();
HMENU hmenu = menu.Detach();
         if (hmenu) DestroyMenu(hmenu);
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
            CStringW text;
            text.Format(L"CPU: %d°C, %d%%\nGPU: %d°C, %d%%",
                m_core.m_nCurTemp[0].load(), m_core.m_nCurDuty[0].load(),
                m_core.m_nCurTemp[1].load(), m_core.m_nCurDuty[1].load());
            SetTray(text);
            m_nTrayLastUpdate = m_core.m_nLastUpdateTime;
        }
    }break;
    }
    return 0;
}

void CFanControlProDlg::OnBnClickedCheckTakeover()
{
    if (m_core.GetStartupState() != StartupState::CoreReady)
    {
        m_ctlTakeOver.SetCheck(FALSE);
        return;
    }
    const BOOL authorized = m_ctlTakeOver.GetCheck();
    m_core.SetUserTakeoverAuthorized(authorized);
    m_core.LockConfig();
    m_core.m_config.TakeOver = authorized;
    m_core.UnlockConfig();
}

void CFanControlProDlg::OnBnClickedCheckForce()
 {
      if (m_ctlForcedCooling.GetCheck() &&
          !CanEnableForcedCooling(m_core.GetStartupState(), m_core.m_userTakeoverAuthorized.load()))
      {
          m_ctlForcedCooling.SetCheck(FALSE);
          AfxMessageBox(L"请先启用风扇接管");
          return;
      }
      m_core.EnableForcedCooling(m_ctlForcedCooling.GetCheck());
      m_core.LockConfig();
      const int controlMode = m_core.m_config.ControlMode;
      m_core.UnlockConfig();
      m_ctlMode.SetCurSel(controlMode);
 }

void CFanControlProDlg::OnBnClickedCheckLinear()
{
    m_core.LockConfig();
    m_core.m_config.Linear = m_ctlLinear.GetCheck();
    m_core.UnlockConfig();
}

void CFanControlProDlg::SetAdvancedMode(BOOL bAdvanced)
{
    const int showAdvanced = DialogLayoutPolicy::ShowAdvancedCommands(bAdvanced) ? SW_SHOW : SW_HIDE;
    const int showMonitoring = DialogLayoutPolicy::ShowMonitoringControls(bAdvanced) ? SW_SHOW : SW_HIDE;
    const int advancedControls[] = {
        IDC_STATIC_ADVANCED_GROUP, IDC_EDIT_INTERVAL, IDC_EDIT_TRANSITION,
        IDC_EDIT_FORCE_TEMP, IDC_CHECK_LINEAR, IDC_CHECK_LOCK_GPU,
        IDC_EDIT_GPU_FREQUENCY, IDC_CHECK_DESKTOP_NOTIFICATIONS,
        IDC_EDIT_WARNING_TEMP, IDC_EDIT_NOTIFICATION_COOLDOWN,
        IDC_CHECK_SILENT, IDC_CHECK_PERFORMANCE, IDC_MAX_DUTY_SLIDER,
        IDC_EDIT_MAX_DUTY, IDC_STATIC_ADVANCED_UPDATE,
        IDC_STATIC_ADVANCED_TRANSITION, IDC_STATIC_ADVANCED_FORCE,
        IDC_STATIC_ADVANCED_FREQUENCY, IDC_STATIC_ADVANCED_WARNING,
        IDC_STATIC_ADVANCED_COOLDOWN, IDC_STATIC_ADVANCED_PRESETS,
        IDC_STATIC_ADVANCED_MAX_DUTY, IDC_BUTTON_LOAD, IDC_BUTTON_SAVE,
        IDC_BUTTON_RESET
    };
    for (int id : advancedControls)
    {
        CWnd* control = GetDlgItem(id);
        if (control) control->ShowWindow(showAdvanced);
    }

    const int monitoringControls[] = {
        IDC_STATIC_MONITOR_GROUP, IDC_STATIC_FAN_GROUP,
        IDC_STATIC_CPU_TEMP, IDC_PROGRESS_CPU_TEMP, IDC_STATIC_GPU_TEMP,
        IDC_PROGRESS_GPU_TEMP, IDC_STATIC_CPU_RPM, IDC_STATIC_GPU_RPM,
        IDC_STATIC_GPU_USAGE, IDC_STATIC_CPU_USAGE, IDC_STATIC_STARTUP_STATUS,
        IDC_STATIC_CONTROL_STATUS, IDC_STATIC_WARNING_STATUS, IDC_COMBO_MODE,
        IDC_CHECK_TAKEOVER, IDC_CHECK_FORCE, IDC_STATIC_CPU_FAN_LABEL,
        IDC_SLIDER_CPU_FAN, IDC_EDIT_CPU_FAN, IDC_STATIC_GPU_FAN_LABEL,
        IDC_SLIDER_GPU_FAN, IDC_EDIT_GPU_FAN, IDC_CHECK_AUTORUN
    };
    for (int id : monitoringControls)
    {
        CWnd* control = GetDlgItem(id);
        if (control) control->ShowWindow(showMonitoring);
    }

    const int targetHeight = DialogLayoutPolicy::SelectWindowHeight(
        bAdvanced, m_nCompactWindowHeight, m_nWindowSize[1]);
    if (m_nWindowSize[0] > 0 && targetHeight > 0)
    {
        CRect rect;
        GetWindowRect(&rect);
        MoveWindow(rect.left, rect.top, m_nWindowSize[0], targetHeight, TRUE);
    }
    m_bAdvancedMode = bAdvanced;
    if (CWnd* advancedButton = GetDlgItem(IDC_BUTTON_ADVANCED))
        advancedButton->SetWindowTextW(bAdvanced ? L"返回监控" : L"高级设置");
}

void CFanControlProDlg::OnBnClickedButtonAdvanced()
{
    SetAdvancedMode(!m_bAdvancedMode);
}

void CFanControlProDlg::ApplyResponsiveLayout()
{
    // 资源控件使用对话框单位，避免按物理像素重排造成高 DPI 截断。
}

void CFanControlProDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
}

void CFanControlProDlg::OnBnClickedCheckAutorun()
{
    int val = m_ctlAutorun.GetCheck();
    if (val)
    {
        int rv = MessageBox(L"请选择开机自动启动方式：\r\n\r\n\"是\"=注册表方式（简单）\r\n\"否\"=任务计划程序（推荐）\r\n\"取消\"=放弃设置",
            L"设置开机自启", MB_YESNOCANCEL);
BOOL bSuccess = FALSE;
         if (IDYES == rv) { bSuccess = CreateAutorunReg(); DeleteAutorunTask(); }
         else if (IDNO == rv) { bSuccess = CreateAutorunTask(); DeleteAutorunReg(); }
         if (!bSuccess) { m_ctlAutorun.SetCheck(FALSE); AfxMessageBox(L"开机自启设置失败，请以管理员身份运行。"); }
         else { m_ctlAutorun.SetCheck(TRUE); }
    }
    else
    {
        DeleteAutorunReg();
        DeleteAutorunTask();
        m_ctlAutorun.SetCheck(QueryAutorunReg() || QueryAutorunTask());
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
        CStringW text;
        text.Format(L"%d%%", pos);
        m_ctlCpuFanEdit.SetWindowTextW(text);
    }
    else if (pScrollBar == (CScrollBar*)&m_ctlGpuFanSlider)
    {
        int pos = m_ctlGpuFanSlider.GetPos();
        m_core.LockConfig();
        m_core.m_config.ManualDuty[1] = pos;
        m_core.UnlockConfig();
        CStringW text;
        text.Format(L"%d%%", pos);
        m_ctlGpuFanEdit.SetWindowTextW(text);
    }
    else if (pScrollBar == (CScrollBar*)&m_ctlMaxDutySlider)
    {
        int pos = m_ctlMaxDutySlider.GetPos();
        m_core.SetMaxDutyLimit(pos);
        CStringW text;
        text.Format(L"%d%%", pos);
        m_ctlMaxDutyEdit.SetWindowTextW(text);
    }
    CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

namespace
{
constexpr PCWSTR kAutorunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr PCWSTR kAutorunValue = L"FanControlPro";
constexpr PCWSTR kAutorunTask = L"FanControlPro";

CStringW CurrentExecutablePath()
{
    return ::GetExePath() + L"FanControlPro.exe";
}
}

BOOL CFanControlProDlg::QueryAutorunReg() const
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kAutorunKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return FALSE;
    DWORD type = 0;
    DWORD size = 0;
    const LONG result = RegQueryValueExW(key, kAutorunValue, nullptr, &type, nullptr, &size);
    if (result != ERROR_SUCCESS || type != REG_SZ || size < sizeof(wchar_t))
    {
        RegCloseKey(key);
        return FALSE;
    }
    std::wstring command(size / sizeof(wchar_t), L'\0');
    const bool read = RegQueryValueExW(key, kAutorunValue, nullptr, &type,
        reinterpret_cast<BYTE*>(&command[0]), &size) == ERROR_SUCCESS;
    RegCloseKey(key);
    if (!read)
        return FALSE;
    command.resize(wcsnlen(command.c_str(), command.size()));
    return IsExactAutorunCommand(command, CurrentExecutablePath().GetString()) ? TRUE : FALSE;
}

BOOL CFanControlProDlg::CreateAutorunReg()
{
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kAutorunKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        return FALSE;
    const CStringW command = L"\"" + CurrentExecutablePath() + L"\"";
    const DWORD bytes = static_cast<DWORD>((command.GetLength() + 1) * sizeof(wchar_t));
    const LONG result = RegSetValueExW(key, kAutorunValue, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(command.GetString()), bytes);
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

BOOL CFanControlProDlg::DeleteAutorunReg()
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kAutorunKey, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        return TRUE;
    const LONG result = RegDeleteValueW(key, kAutorunValue);
    RegCloseKey(key);
    return result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
}

BOOL CFanControlProDlg::RunHiddenCommand(PCWSTR commandLine, DWORD* exitCode) const
{
    std::vector<wchar_t> command(commandLine, commandLine + wcslen(commandLine) + 1);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
        nullptr, nullptr, &startup, &process))
        return FALSE;
    const DWORD waitResult = WaitForSingleObject(process.hProcess, 10000);
    if (waitResult == WAIT_OBJECT_0 && exitCode)
        GetExitCodeProcess(process.hProcess, exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return waitResult == WAIT_OBJECT_0;
}

BOOL CFanControlProDlg::QueryAutorunTask() const
{
    DWORD exitCode = 1;
    return RunHiddenCommand(L"schtasks.exe /Query /TN \"FanControlPro\"", &exitCode) && exitCode == 0;
}

BOOL CFanControlProDlg::CreateAutorunTask()
{
    const CStringW xmlPath = ::GetExePath() + L"FanControlPro.task.xml";
    if (!CreateTaskXml(xmlPath, CurrentExecutablePath()))
        return FALSE;
    CStringW command;
    command.Format(L"schtasks.exe /Create /F /XML \"%s\" /TN \"FanControlPro\"", xmlPath.GetString());
    DWORD exitCode = 1;
    const BOOL created = RunHiddenCommand(command, &exitCode) && exitCode == 0;
    DeleteFileW(xmlPath);
    return created;
}

BOOL CFanControlProDlg::DeleteAutorunTask()
{
    DWORD exitCode = 1;
    return RunHiddenCommand(L"schtasks.exe /Delete /F /TN \"FanControlPro\"", &exitCode) &&
        (exitCode == 0 || !QueryAutorunTask());
}

BOOL CFanControlProDlg::CreateTaskXml(PCWSTR xmlPath, PCWSTR targetPath) const
{
    CStringW xml;
    xml.Format(L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
        L"<Task version=\"1.2\" xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\r\n"
        L"<Triggers><LogonTrigger><Enabled>true</Enabled></LogonTrigger></Triggers>\r\n"
        L"<Principals><Principal id=\"Author\"><GroupId>S-1-5-32-545</GroupId>"
        L"<RunLevel>HighestAvailable</RunLevel></Principal></Principals>\r\n"
        L"<Settings><MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy><Enabled>true</Enabled>"
        L"<Hidden>true</Hidden><ExecutionTimeLimit>PT0S</ExecutionTimeLimit></Settings>\r\n"
        L"<Actions Context=\"Author\"><Exec><Command>%s</Command></Exec></Actions>\r\n</Task>\r\n", targetPath);
    const int byteCount = WideCharToMultiByte(CP_UTF8, 0, xml, xml.GetLength(), nullptr, 0, nullptr, nullptr);
    if (byteCount <= 0)
        return FALSE;
    std::vector<char> bytes(byteCount);
    WideCharToMultiByte(CP_UTF8, 0, xml, xml.GetLength(), bytes.data(), byteCount, nullptr, nullptr);
    HANDLE file = CreateFileW(xmlPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    DWORD written = 0;
    const BOOL success = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size();
    CloseHandle(file);
    return success;
}

CString CFanControlProDlg::GetExePath()
{
    return ::GetExePath();
}

BOOL CFanControlProDlg::CheckInputFrequency(int nFrequency)
{
    if (nFrequency < 0 || nFrequency > m_core.m_GpuInfo.m_nMaxFrequency)
    {
        CStringW text;
        text.Format(L"GPU 频率必须为 0-%d", m_core.m_GpuInfo.m_nMaxFrequency);
        AfxMessageBox(text);
        return FALSE;
    }
    return TRUE;
}
