#include "stdafx.h"
#include "Core.h"
#include "resource.h"
#include <cmath>
#include <strsafe.h>
using std::max;
using std::min;

// 获取当前时间
int GetTime(tm *pt, int offset)
{
    tm t = { 0 };
    time_t tt;
    if (!pt)
        pt = &t;
    time(&tt);
    tt += offset;
    localtime_s(pt, &tt);
    return (pt->tm_hour * 10000 + pt->tm_min * 100 + pt->tm_sec);
}

// 计算时间差
int GetTimeInterval(int a, int b, int *p)
{
    int a1 = a / 10000;
    int a2 = (a % 10000) / 100;
    int a3 = a % 100;

    int b1 = b / 10000;
    int b2 = (b % 10000) / 100;
    int b3 = b % 100;

    int c = (a1 - b1) * 3600 + (a2 - b2) * 60 + a3 - b3;
    if (p)
        *p = c;
    int sgn = (c >= 0) ? 1 : -1;
    c = abs(c);

    int d = (c / 3600) * 10000 + (c % 3600) / 60 * 100 + c % 60;
    return d * sgn;
}

// 获取 exe 路径
CStringW GetExePath()
{
    wchar_t pathbuf[MAX_PATH] = {};
    const DWORD pathlen = ::GetModuleFileNameW(nullptr, pathbuf, _countof(pathbuf));
    if (pathlen == 0 || pathlen >= _countof(pathbuf))
        return L"";

    wchar_t* lastSlash = wcsrchr(pathbuf, L'\\');
    if (lastSlash == nullptr)
        return L"";
    *(lastSlash + 1) = L'\0';
    return CStringW(pathbuf);
}

// ==================== CGPUInfo 实现 ====================

 CGPUInfo::CGPUInfo()
 {
     // ── 所有成员初始化兜底（DLL 加载失败时也要有合法值）──
     m_nMaxFrequency      = 0;
     m_nStandardFrequency = 0;
     m_nGraphicsClock     = 0;
     m_nMemoryClock       = 0;
     m_nUsage             = 0;
     m_nLockClock         = -1;
     m_nBaseClock         = 0;
     m_nBoostClock        = 0;
     m_sName              = nullptr;
     m_nDeviceID          = 0;
     m_nGraphicsRangeMax  = 0;
     m_nGraphicsRangeMin  = 0;
     m_nMemoryRangeMax    = 0;
     m_nMemoryRangeMin    = 0;
     m_pfnInitGPU_API              = NULL;
     m_pfnSet_GPU_Number           = NULL;
     m_pfnGet_GPU_Base_Clock       = NULL;
     m_pfnGet_GPU_Boost_Clock      = NULL;
     m_pfnCheck_GPU_VRAM_Clock     = NULL;
     m_pfnGet_GPU_Graphics_Clock   = NULL;
     m_pfnGet_GPU_Memory_Clock     = NULL;
     m_pfnGet_Memory_OC_max        = NULL;
     m_pfnGet_GPU_Util             = NULL;
     m_pfnGet_GPU_name             = NULL;
     m_pfnGet_GPU_TotalNumber      = NULL;
     m_pfnGet_GPU_Overclock_range  = NULL;
     m_pfnGet_Memory_range         = NULL;
     m_pfnGet_GPU_Overclock_rangeMax = NULL;
     m_pfnGet_GPU_Overclock_rangeMin = NULL;
     m_pfnGet_Memory_range_max     = NULL;
     m_pfnGet_Memory_range_min     = NULL;
     m_pfnGet_NVDeviceID           = NULL;
     m_pfnLock_Frequency           = NULL;
     m_pfnLock_Frequency_MEM       = NULL;
     m_pfnSet_CoreOC               = NULL;
     m_pfnSet_MEMOC                = NULL;
     m_pfnCloseGPU_API             = NULL;
     
    const CStringW dllpth = GetExePath() + L"NVGPU_DLL.dll";
    if (!m_hGPUdll.Load(dllpth))
    {
        TRACE0("无法加载 NVGPU_DLL.dll\n");
        return;
    }

HMODULE hDll = m_hGPUdll.Get();
     // 加载函数指针
     m_pfnInitGPU_API          = (In_0_Out_n_Func *)::GetProcAddress(hDll, "InitGPU_API");
     m_pfnSet_GPU_Number       = (In_1_Out_n_Func *)::GetProcAddress(hDll, "Set_GPU_Number");
     m_pfnGet_GPU_Base_Clock   = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_GPU_Base_Clock");
     m_pfnGet_GPU_Boost_Clock  = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_GPU_Boost_Clock");
     m_pfnCheck_GPU_VRAM_Clock = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Check_GPU_VRAM_Clock");
     m_pfnGet_GPU_Graphics_Clock = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_GPU_Graphics_Clock");
     m_pfnGet_GPU_Memory_Clock = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_GPU_Memory_Clock");
     m_pfnGet_Memory_OC_max    = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_Memory_OC_max");
     m_pfnGet_GPU_Util         = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_GPU_Util");
     m_pfnGet_GPU_name         = (In_0_Out_s_Func *)::GetProcAddress(hDll, "Get_GPU_name");
     m_pfnGet_GPU_TotalNumber  = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_GPU_TotalNumber");
     m_pfnGet_GPU_Overclock_range  = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_GPU_Overclock_range");
     m_pfnGet_Memory_range         = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_Memory_range");
     m_pfnGet_GPU_Overclock_rangeMax  = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_GPU_Overclock_rangeMax");
     m_pfnGet_GPU_Overclock_rangeMin  = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_GPU_Overclock_rangeMin");
     m_pfnGet_Memory_range_max   = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_Memory_range_max");
     m_pfnGet_Memory_range_min   = (In_0_Out_n_Func *)::GetProcAddress(hDll, "Get_Memory_range_min");
     m_pfnGet_NVDeviceID     = (In_1_Out_n_Func *)::GetProcAddress(hDll, "Get_NVDeviceID");
     m_pfnLock_Frequency     = (In_2_Out_n_Func *)::GetProcAddress(hDll, "Lock_Frequency");
     m_pfnLock_Frequency_MEM = (In_2_Out_n_Func *)::GetProcAddress(hDll, "Lock_Frequency_MEM");
     m_pfnSet_CoreOC         = (In_2_Out_n_Func *)::GetProcAddress(hDll, "Set_CoreOC");
     m_pfnSet_MEMOC          = (In_2_Out_n_Func *)::GetProcAddress(hDll, "Set_MEMOC");
     m_pfnCloseGPU_API       = (In_0_Out_0_Func *)::GetProcAddress(hDll, "CloseGPU_API");
     // ── DLL 安全检查：所有调用路径上的函数指针必须非空 ──
     if (!m_pfnInitGPU_API || !m_pfnCheck_GPU_VRAM_Clock || !m_pfnCloseGPU_API ||
         !m_pfnGet_GPU_Graphics_Clock || !m_pfnGet_GPU_Memory_Clock || !m_pfnGet_GPU_Util ||
         !m_pfnSet_CoreOC || !m_pfnSet_MEMOC || !m_pfnLock_Frequency || !m_pfnLock_Frequency_MEM ||
         !m_pfnSet_GPU_Number || !m_pfnGet_GPU_Base_Clock || !m_pfnGet_GPU_Boost_Clock ||
         !m_pfnGet_GPU_name || !m_pfnGet_NVDeviceID ||
         !m_pfnGet_GPU_Overclock_rangeMax || !m_pfnGet_GPU_Overclock_rangeMin ||
         !m_pfnGet_Memory_range_max || !m_pfnGet_Memory_range_min)
     {
         TRACE0("NVGPU_DLL.dll 缺少必需的导出函数，GPU 功能不可用\n");
         m_hGPUdll.Close();
         return;
     }
     if (m_pfnInitGPU_API())
     {
         TRACE0("InitGPU_API 初始化失败。\n");
         m_hGPUdll.Close();
         return;
     }
     m_pfnSet_GPU_Number(0);
    m_nBaseClock = m_pfnGet_GPU_Base_Clock();
    m_nBoostClock = m_pfnGet_GPU_Boost_Clock();
    m_sName = m_pfnGet_GPU_name();
    m_nDeviceID = m_pfnGet_NVDeviceID(0);
    m_nGraphicsRangeMax = m_pfnGet_GPU_Overclock_rangeMax();
    m_nGraphicsRangeMin = m_pfnGet_GPU_Overclock_rangeMin();
    m_nMemoryRangeMax = m_pfnGet_Memory_range_max();
    m_nMemoryRangeMin = m_pfnGet_Memory_range_min();

    m_nStandardFrequency = m_nBoostClock - m_nGraphicsRangeMin;
    m_nMaxFrequency = m_nStandardFrequency + m_nGraphicsRangeMax;
    m_nLockClock = -1;

    Update();
    TRACE0("成功加载 NVGPU_DLL.dll\n");
}

CGPUInfo::~CGPUInfo()
{
    if (m_hGPUdll)
    {
        LockFrequency();
        m_pfnCloseGPU_API();
    }
}

BOOL CGPUInfo::Update()
{
    if (!m_hGPUdll)
        return FALSE;
    if (!m_pfnCheck_GPU_VRAM_Clock())
        return FALSE;
    m_nGraphicsClock = m_pfnGet_GPU_Graphics_Clock();
    m_nMemoryClock = m_pfnGet_GPU_Memory_Clock();
    m_nUsage = m_pfnGet_GPU_Util();
    return TRUE;
}

BOOL CGPUInfo::LockFrequency(int frequency)
{
    if (!m_hGPUdll)
        return FALSE;
    if (frequency < 0 || frequency > m_nMaxFrequency)
        return FALSE;
    if (frequency == 0)
        frequency = m_nStandardFrequency;
    if (m_nLockClock == frequency)
        return TRUE;
    m_nLockClock = frequency;

    int GpuOverclock = 0;
    int MemOverclock = 0;
    int GpuClock = 0;

    if (frequency > 0 && frequency < m_nStandardFrequency)
    {
        GpuClock = frequency;
    }
    else if (frequency > m_nStandardFrequency)
    {
        if (m_nGraphicsRangeMax <= 0)
            return FALSE;
        GpuOverclock = frequency - m_nStandardFrequency;
        MemOverclock = GpuOverclock * m_nMemoryRangeMax / m_nGraphicsRangeMax;
    }

    m_pfnSet_CoreOC(0, GpuOverclock);
    m_pfnSet_MEMOC(0, MemOverclock);
    m_pfnLock_Frequency(0, GpuClock);
    m_pfnLock_Frequency_MEM(0, 0);
    
    return TRUE;
}

// ==================== CConfig 实现 ====================

CConfig::CConfig()
{
    const CStringW path = GetExePath() + L"FanControlPro.cfg";
    StringCchCopyW(ConfigPath, _countof(ConfigPath), path);
    LoadDefault();
}

void CConfig::LoadDefault()
{
    int i = 0;
    DutyList[0][i++] = 85;
    DutyList[0][i++] = 80;
    DutyList[0][i++] = 70;
    DutyList[0][i++] = 55;
    DutyList[0][i++] = 40;
    DutyList[0][i++] = 30;
    DutyList[0][i++] = 25;
    DutyList[0][i++] = 20;
    DutyList[0][i++] = 15;
    DutyList[0][i++] = 10;

    for (int j = 0; j < TEMP_LEVELS; j++)
        DutyList[1][j] = DutyList[0][j];
    
    DutyList[1][8] = 12;
    DutyList[1][9] = 8;

    TransitionTemp = 3;
    UpdateInterval = 2;
    Linear = FALSE;
    // 首次启动保持 BIOS 控制，用户确认传感器读数正常后再显式启用接管。
    TakeOver = FALSE;
    // 55°C 会让 13500H/4060 在日常使用中几乎一直处于全速；设为安全而实用的默认值。
    ForceTemp = 85;
    MaxDutyLimit = MAX_FAN_DUTY_LIMIT;
    
    LockGPUFrequency = FALSE;
    GPUFrequency = 0;
    
    ControlMode = 0;
    ManualDuty[0] = 50;
    ManualDuty[1] = 50;

    WarningTemp = 90;
    DesktopNotifications = TRUE;
    NotificationCooldownMinutes = 10;
}

void WriteDiagnosticLog(PCSTR message)
{
    const CStringW path = GetExePath() + L"FanControlPro.debug.log";
    FILE* file = _wfopen(path, L"at");
    if (!file)
        return;

    SYSTEMTIME now = {};
    GetLocalTime(&now);
    fprintf(file, "%04u-%02u-%02u %02u:%02u:%02u %s\n", now.wYear, now.wMonth,
        now.wDay, now.wHour, now.wMinute, now.wSecond, message);
    fclose(file);
}

void CConfig::Normalize()
{
    TransitionTemp = max(0, min(10, TransitionTemp));
    UpdateInterval = max(1, min(5, UpdateInterval));
    ForceTemp = max(60, min(95, ForceTemp));
    MaxDutyLimit = max(0, min(100, MaxDutyLimit));
    ControlMode = max(0, min(2, ControlMode));
    WarningTemp = max(60, min(100, WarningTemp));
    DesktopNotifications = !!DesktopNotifications;
    NotificationCooldownMinutes = max(1, min(60, NotificationCooldownMinutes));
    Linear = !!Linear;
    TakeOver = !!TakeOver;
    LockGPUFrequency = !!LockGPUFrequency;
    GPUFrequency = max(0, GPUFrequency);
    for (int fan = 0; fan < MAX_EC_FANS; ++fan)
    {
        ManualDuty[fan] = max(0, min(100, ManualDuty[fan]));
        for (int level = 0; level < TEMP_LEVELS; ++level)
            DutyList[fan][level] = max(0, min(100, DutyList[fan][level]));
    }
}

namespace
{
ConfigV4Disk ToDiskConfig(const CConfig& config)
{
    ConfigV4Disk disk{};
    std::memcpy(disk.DutyList, config.DutyList, sizeof(disk.DutyList));
    disk.TransitionTemp = config.TransitionTemp;
    disk.UpdateInterval = config.UpdateInterval;
    disk.Linear = config.Linear;
    disk.TakeOver = config.TakeOver;
    disk.ForceTemp = config.ForceTemp;
    disk.MaxDutyLimit = config.MaxDutyLimit;
    disk.LockGPUFrequency = config.LockGPUFrequency;
    disk.GPUFrequency = config.GPUFrequency;
    disk.ControlMode = config.ControlMode;
    std::memcpy(disk.ManualDuty, config.ManualDuty, sizeof(disk.ManualDuty));
    disk.WarningTemp = config.WarningTemp;
    disk.DesktopNotifications = config.DesktopNotifications;
    disk.NotificationCooldownMinutes = config.NotificationCooldownMinutes;
    return disk;
}

void ApplyDiskConfig(CConfig& config, const ConfigV4Disk& disk)
{
    std::memcpy(config.DutyList, disk.DutyList, sizeof(config.DutyList));
    config.TransitionTemp = disk.TransitionTemp;
    config.UpdateInterval = disk.UpdateInterval;
    config.Linear = disk.Linear;
    config.TakeOver = disk.TakeOver;
    config.ForceTemp = disk.ForceTemp;
    config.MaxDutyLimit = disk.MaxDutyLimit;
    config.LockGPUFrequency = disk.LockGPUFrequency;
    config.GPUFrequency = disk.GPUFrequency;
    config.ControlMode = disk.ControlMode;
    std::memcpy(config.ManualDuty, disk.ManualDuty, sizeof(config.ManualDuty));
    config.WarningTemp = disk.WarningTemp;
    config.DesktopNotifications = disk.DesktopNotifications;
    config.NotificationCooldownMinutes = disk.NotificationCooldownMinutes;
    config.Normalize();
}
}

void CConfig::LoadConfig()
{
    ConfigV4Disk disk{};
    if (!ReadConfigWithBackup(ConfigPath, &disk, sizeof(disk)))
    {
        LoadDefault();
        SaveConfig();
        return;
    }
    ApplyDiskConfig(*this, disk);
}

void CConfig::SaveConfig()
{
    const ConfigV4Disk disk = ToDiskConfig(*this);
    if (!WriteConfigAtomically(ConfigPath, &disk, sizeof(disk)))
        WriteDiagnosticLog("Config write failed");
}

void CConfig::ExportConfig(PCWSTR path) const
{
    const ConfigV4Disk disk = ToDiskConfig(*this);
    if (!WriteConfigAtomically(path, &disk, sizeof(disk)))
        AfxMessageBox(L"无法打开导出路径");
}

void CConfig::ImportConfig(PCWSTR path)
{
    ConfigV4Disk disk{};
    if (!ReadConfigWithBackup(path, &disk, sizeof(disk)))
    {
        AfxMessageBox(L"配置文件格式不匹配或版本不兼容，导入失败");
        return;
    }
    ApplyDiskConfig(*this, disk);
}

// ==================== CCore 实现 ====================

CCore::CCore()
{
    m_pfnInitIo = NULL;
    m_pfnSetFanDuty = NULL;
    m_pfnSetFANDutyAuto = NULL;
    m_pfnGetTempFanDuty = NULL;
    m_pfnGetFANCounter = NULL;
    m_pfnGetECVersion = NULL;
    m_pfnGetFANRPM[0] = NULL;
    m_pfnGetFANRPM[1] = NULL;
    
    m_nInit = 0;
    m_nExit = 0;
    
    for (int i = 0; i < 2; i++)
    {
        m_nCurTemp[i] = 0;
        m_nLastTemp[i] = 0;
        m_nSetDuty[i] = 0;
        m_nSetDutyLevel[i] = 0;
        m_nCurDuty[i] = 0;
        m_nCurRPM[i] = 0;
        m_nTargetDuty[i] = 0;
        m_nReadbackDuty[i] = 0;
    }
    
    m_bUpdateRPM = FALSE;
    m_nLastUpdateTime = GetTime(0, -5);
    m_bForcedCooling = FALSE;
    m_bTakeOverStatus = FALSE;
    m_bForcedRefresh = FALSE;
    m_nNextCheckTime = 0;
    m_bSetPriority = FALSE;
    
    m_nSmoothedDuty[0] = 0;
    m_nSmoothedDuty[1] = 0;
    
    m_bTempWarning = FALSE;
    m_bThermalEmergency = FALSE;
    m_hWnd = NULL;

    m_nLastSetDutyEC[0] = 0;
    m_nLastSetDutyEC[1] = 0;
m_nEcTakeoverCount = 0;
     m_bEcTakeoverFlag = FALSE;
     m_nSavedControlMode = 0;

     m_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
     m_startupCheckResult = std::make_shared<StartupCheckResult>();

     InitializeCriticalSection(&m_csConfig);
 }

 CCore::~CCore()
 {
     Uninit();
     if (m_hExitEvent) { CloseHandle(m_hExitEvent); m_hExitEvent = NULL; }
     DeleteCriticalSection(&m_csConfig);
 }

void CCore::SaveConfigSnapshot()
{
    CConfig snapshot;
    LockConfig();
    snapshot = m_config;
    UnlockConfig();

    std::lock_guard<std::mutex> lock(m_configPersistenceMutex);
    snapshot.SaveConfig();
}

BOOL CCore::Init()
{
    if (m_hInstDLL)
        return TRUE;

    WriteDiagnosticLog("Core.Init started");
    TRACE0("内核开始初始化\n");
    m_nInit = -1;
    
    const CStringW dllpth = GetExePath() + L"ClevoEcInfo.dll";
    if (!m_hInstDLL.Load(dllpth))
    {
        char logLine[128] = {};
        sprintf_s(logLine, "ClevoEcInfo.dll load failed. LastError=%lu", GetLastError());
        WriteDiagnosticLog(logLine);
        m_initError = GetLastError();
        m_ecDllAvailable = FALSE;
        m_driverInitialized = FALSE;
        m_startupState = StartupState::CoreFailed;
        SetStartupCheckFailure(L"依赖 DLL 无法加载", L"依赖 DLL 无法加载，仅监控模式");
        if (m_hWnd) PostMessage(m_hWnd, WM_CORE_INIT_RESULT, 0, 0);
        return FALSE;
    }
    m_ecDllAvailable = TRUE;
    WriteDiagnosticLog("ClevoEcInfo.dll loaded");

    HMODULE hDll = m_hInstDLL.Get();
    m_pfnInitIo = (InitIo *)::GetProcAddress(hDll, "InitIo");
    m_pfnSetFanDuty = (FnSetFanDuty *)::GetProcAddress(hDll, "SetFanDuty");
    m_pfnSetFANDutyAuto = (SetFANDutyAuto *)::GetProcAddress(hDll, "SetFanDutyAuto");
    m_pfnGetTempFanDuty = (GetTempFanDuty *)::GetProcAddress(hDll, "GetTempFanDuty");
    m_pfnGetFANCounter = (GetFANCounter *)::GetProcAddress(hDll, "GetFanCount");
    m_pfnGetECVersion = (GetECVersion *)::GetProcAddress(hDll, "GetECVersion");
    m_pfnGetFANRPM[0] = (GetFanRpm *)::GetProcAddress(hDll, "GetCpuFanRpm");
    m_pfnGetFANRPM[1] = (GetFanRpm *)::GetProcAddress(hDll, "GetGpuFanRpm");

    const BOOL hasRequiredExports = m_pfnInitIo != NULL && m_pfnSetFanDuty != NULL &&
        m_pfnSetFANDutyAuto != NULL && m_pfnGetTempFanDuty != NULL;
    if (!hasRequiredExports)
    {
        m_hInstDLL.Close();
        m_initError = ERROR_PROC_NOT_FOUND;
        m_driverInitialized = FALSE;
        m_startupState = StartupState::CoreFailed;
        SetStartupCheckFailure(L"EC DLL 缺少必要导出接口", L"EC 接口不完整，仅监控模式");
        if (m_hWnd) PostMessage(m_hWnd, WM_CORE_INIT_RESULT, 0, 0);
        return FALSE;
    }
    const BOOL initOk = m_pfnInitIo() == 1;
    char logLine[160] = {};
    sprintf_s(logLine, "EC exports=%d InitIo=%d LastError=%lu", hasRequiredExports, initOk, GetLastError());
    WriteDiagnosticLog(logLine);
    if (!initOk)
    {
        m_hInstDLL.Close();
        m_initError = GetLastError();
        m_driverInitialized = FALSE;
        m_startupState = StartupState::CoreFailed;
        SetStartupCheckFailure(L"底层驱动初始化失败", L"底层驱动初始化失败，仅监控模式");
        if (m_hWnd) PostMessage(m_hWnd, WM_CORE_INIT_RESULT, 0, 0);
        return FALSE;
    }

    TRACE0("内核初始化成功\n");
    
    m_nInit = 1;
    m_startupState = StartupState::SelfChecking;
    m_driverInitialized = TRUE;
    m_initError = ERROR_SUCCESS;
    WriteDiagnosticLog("Core.Init completed read-only probe");
    if (m_hWnd) PostMessage(m_hWnd, WM_CORE_INIT_RESULT, 2, 0);
    return TRUE;
}

void CCore::Uninit()
{
    m_hInstDLL.Close();
    m_nInit = 0;
}

void CCore::Run()
{
    m_startupState = StartupState::CoreStarting;
    WriteDiagnosticLog("Core config load started");
    EnterCriticalSection(&m_csConfig);
    m_config.LoadConfig();
    m_config.TakeOver = FALSE;
    m_config.LockGPUFrequency = FALSE;
    LeaveCriticalSection(&m_csConfig);
    WriteDiagnosticLog("Core config load completed");
    
    if (!m_nInit)
        Init();

    if (m_nInit == 1)
    {
        TRACE0("内核开始运行\n");
        ULONGLONG nextCheckTick = 0;
        int ecRefreshTick = 0;
        while (!m_nExit)
        {
            if (m_takeoverSessionResetRequested.exchange(false))
            {
                m_takeoverVerification.Reset();
                m_controlVerification = m_userTakeoverAuthorized.load()
                    ? ControlVerification::RequestingTakeover
                    : ControlVerification::BiosControl;
            }
            if (m_resetFansRequested.exchange(false))
                ResetFan();

            const ULONGLONG now = GetTickCount64();
            
            BOOL  bTakeOver;
            int   nUpdateInterval;
            EnterCriticalSection(&m_csConfig);
            bTakeOver = m_config.TakeOver;
            nUpdateInterval = m_config.UpdateInterval;
            LeaveCriticalSection(&m_csConfig);
            
            if (++ecRefreshTick >= EC_REFRESH_TICKS)
            {
                ecRefreshTick = 0;
                if (bTakeOver && CanWriteFans(m_startupState.load(), m_userTakeoverAuthorized.load()) &&
                    m_controlVerification.load() == ControlVerification::Active)
                    SetFanDuty();
            }
            
            if (now >= nextCheckTick || m_bForcedRefresh.exchange(FALSE))
            {
                Work();
                m_nLastUpdateTime = static_cast<int>(now / 1000);
                nextCheckTick = now + static_cast<ULONGLONG>(nUpdateInterval) * 1000;
                
                if (!m_bSetPriority)
                {
                    m_bSetPriority = TRUE;
                    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
                }
}
             WaitForSingleObject(m_hExitEvent, 100);
         }
        TRACE0("内核结束运行\n");
        ResetFan();
    }
    m_nExit = 2;
}

void CCore::Work()
{
    const BOOL sensorsValid = Update();
    if (m_sensorHealth.IsFaulted())
    {
        m_controlVerification = ControlVerification::Fault;
        ResetFan();
        return;
    }
    if (!sensorsValid)
        return;

    CheckTempWarning();

    if (m_takeoverVerification.IsFaulted())
    {
        ResetFan();
        return;
    }

    VerifyAndReclaim();
    if (m_takeoverVerification.IsFaulted())
    {
        ResetFan();
        return;
    }
    
    CConfig cfgSnap;
     BOOL bForced;
     EnterCriticalSection(&m_csConfig);
     cfgSnap = m_config;
      bForced = m_bForcedCooling.load();
      LeaveCriticalSection(&m_csConfig);

      const int hottestTemp = max(m_nCurTemp[0].load(), m_nCurTemp[1].load());
      if (!m_bThermalEmergency)
          m_bThermalEmergency = hottestTemp >= cfgSnap.ForceTemp;
      else
          m_bThermalEmergency = hottestTemp >= cfgSnap.ForceTemp - THERMAL_RECOVERY_GAP;

      if (bForced || m_bThermalEmergency)
     {
         m_nSetDuty[0] = FORCED_COOLING_DUTY;
        m_nSetDutyLevel[0] = 10;
         m_nSetDuty[1] = FORCED_COOLING_DUTY;
         m_nSetDutyLevel[1] = 10;
         // 紧急温度不能受 4% 平滑升速限制，否则最高需近一分钟才达到满速。
         m_nSmoothedDuty[0] = FORCED_COOLING_DUTY;
         m_nSmoothedDuty[1] = FORCED_COOLING_DUTY;
         SetFanDuty();
         return;
    }
    
    if (cfgSnap.TakeOver)
        Control(cfgSnap);
    else
        ResetFan();

    if (cfgSnap.LockGPUFrequency)
        m_GpuInfo.LockFrequency(cfgSnap.GPUFrequency);
    else
        m_GpuInfo.LockFrequency(0);
}

BOOL CCore::Update()
{
    if (!m_pfnGetTempFanDuty)
        return FALSE;

    ECData samples[MAX_EC_FANS] = {};
    bool validTemperature[MAX_EC_FANS] = {};
    for (int i = 0; i < MAX_EC_FANS; ++i)
    {
        const int previousTemp = m_nCurTemp[i].load();
        for (int attempt = 0; attempt < 3; ++attempt)
        {
            samples[i] = m_pfnGetTempFanDuty(i + 1);
            const int sampledTemp = static_cast<int>(samples[i].Remote);
            validTemperature[i] = sampledTemp != 0 && sampledTemp <= MAX_VALID_TEMPERATURE &&
                (previousTemp == 0 || abs(sampledTemp - previousTemp) <= 30);
            if (validTemperature[i])
                break;

            if (attempt < 2 && WaitForSingleObject(m_hExitEvent, 1000) == WAIT_OBJECT_0)
                return FALSE;
        }
    }

    if (!m_sensorHealth.RecordCycle(validTemperature[0], validTemperature[1]))
        return FALSE;
    if (!validTemperature[0] || !validTemperature[1])
    {
        RecordStartupCheck(static_cast<int>(samples[0].Remote), static_cast<int>(samples[1].Remote));
        TRACE("温度传感器异常：本轮采样已跳过，暂不更新风扇控制\n");
        return FALSE;
    }

    for (int i = 0; i < MAX_EC_FANS; ++i)
    {
        const int previousTemp = m_nCurTemp[i].load();
        m_nLastTemp[i] = previousTemp;
        m_nCurTemp[i] = static_cast<int>(samples[i].Remote);
        m_nCurDuty[i] = int(samples[i].FanDuty * 100 / double(EC_FAN_DUTY_MAX) + 0.5);
        m_nReadbackDuty[i] = m_nCurDuty[i].load();

        if (m_bUpdateRPM.load() && m_pfnGetFANRPM[i] != NULL)
        {
            const int value = m_pfnGetFANRPM[i]();
            if (value == 0)
                m_nCurRPM[i] = 0;
            else if (value > RPM_MIN_PULSE && value < RPM_MAX_PULSE)
                m_nCurRPM[i] = RPM_PULSE_FACTOR / value;
            else
                m_nCurRPM[i] = -1;
        }
        else
        {
            m_nCurRPM[i] = -1;
        }
    }

    m_takeoverVerification.RecordReadback(
        m_nReadbackDuty[0].load(), m_nReadbackDuty[1].load(), GetTickCount64());
    m_controlVerification = m_takeoverVerification.State();

    if (m_bUpdateRPM.load())
        m_GpuInfo.Update();
    RecordStartupCheck(m_nCurTemp[0].load(), m_nCurTemp[1].load());
    return TRUE;
}

void CCore::Control(const CConfig& cfg)
{
    switch (cfg.ControlMode)
    {
    case 0:
        if (cfg.Linear)
            CalcLinearDuty(cfg);
        else
            CalcStdDuty(cfg);
        break;
    case 1:
        CalcManualDuty(cfg);
        break;
    case 2:
        m_nSetDuty[0] = FORCED_COOLING_DUTY;
        m_nSetDuty[1] = FORCED_COOLING_DUTY;
        m_nSetDutyLevel[0] = 10;
        m_nSetDutyLevel[1] = 10;
        break;
    }
    
    SetFanDuty();
}

void CCore::CalcLinearDuty(const CConfig& cfg)
{
    int duty, dl;

    for (int i = 0; i < 2; i++)
    {
        m_nLastTemp[i] = max(m_nLastTemp[i], m_nCurTemp[i].load());
        m_nLastTemp[i] = min(m_nLastTemp[i], m_nCurTemp[i].load() + cfg.TransitionTemp);

        int j = m_nLastTemp[i];

        if (j < 45)
        {
            duty = cfg.DutyList[i][TEMP_LEVELS - 1];
            dl = 0;
        }
        else if (j >= 90)
        {
            duty = cfg.DutyList[i][0];
            dl = TEMP_LEVELS;
        }
        else
        {
            int idx = 0;
            if (j < 50) idx = 8;
            else if (j < 55) idx = 7;
            else if (j < 60) idx = 6;
            else if (j < 65) idx = 5;
            else if (j < 70) idx = 4;
            else if (j < 75) idx = 3;
            else if (j < 80) idx = 2;
            else if (j < 85) idx = 1;
            else idx = 0;

            int temp_l = TEMP_LIST[idx + 1];
            int temp_h = TEMP_LIST[idx];
            int duty_l = cfg.DutyList[i][idx + 1];
            int duty_h = cfg.DutyList[i][idx];
            
            duty = int((duty_h - duty_l) / double(temp_h - temp_l) * (j - temp_l) + 0.5) + duty_l;
            dl = TEMP_LEVELS - idx;
        }
        
        duty = min(duty, cfg.MaxDutyLimit);
        
        m_nSetDuty[i] = duty;
        m_nSetDutyLevel[i] = dl;
    }
}

void CCore::CalcStdDuty(const CConfig& cfg)
{
    int dl;
    int last_dl;
    
    for (int i = 0; i < 2; i++)
    {
        int j = m_nCurTemp[i];
        last_dl = m_nSetDutyLevel[i];
        
        int k;
        for (k = 0; k < TEMP_LEVELS; k++)
        {
            dl = TEMP_LEVELS - k;
            if (j >= TEMP_LIST[k])
            {
                break;
            }
            else if (j < TEMP_LIST[k] - cfg.TransitionTemp)
            {
                continue;
            }
            else
            {
                if (last_dl >= dl)
                {
                    break;
                }
                continue;
            }
        }
        k = min(TEMP_LEVELS - 1, k);
        
        int duty = min(cfg.DutyList[i][k], cfg.MaxDutyLimit);
        
        m_nSetDuty[i] = duty;
        m_nSetDutyLevel[i] = dl;
    }
}

void CCore::CalcManualDuty(const CConfig& cfg)
{
    for (int i = 0; i < 2; i++)
    {
        int duty = min(cfg.ManualDuty[i], cfg.MaxDutyLimit);
        m_nSetDuty[i] = duty;
        m_nSetDutyLevel[i] = duty / 10;
    }
}

void CCore::ResetFan()
{
    if (m_fansTouched.exchange(false) && m_pfnSetFANDutyAuto != NULL)
    {
        int fanCount = 2;
        if (m_pfnGetFANCounter)
            fanCount = min(MAX_EC_FANS, max(1, m_pfnGetFANCounter()));
        for (int i = 0; i < fanCount; i++)
            m_pfnSetFANDutyAuto(i + 1);
        m_bTakeOverStatus = FALSE;
        m_nLastSetDutyEC[0] = 0;
        m_nLastSetDutyEC[1] = 0;
        m_nTargetDuty[0] = 0;
        m_nTargetDuty[1] = 0;
        
        m_nSmoothedDuty[0] = 0;
        m_nSmoothedDuty[1] = 0;
    }
}

void CCore::VerifyAndReclaim()
{
    if (!m_bTakeOverStatus || m_takeoverVerification.IsFaulted() ||
        m_pfnGetTempFanDuty == NULL || m_pfnSetFanDuty == NULL ||
        !CanWriteFans(m_startupState.load(), m_userTakeoverAuthorized.load()))
        return;

    m_bEcTakeoverFlag = FALSE;
    bool needsReclaim[MAX_EC_FANS] = {};
    for (int i = 0; i < 2; i++)
    {
        if (m_nLastSetDutyEC[i] <= 0) continue;

        ECData data = m_pfnGetTempFanDuty(i + 1);
        int curDutyEC = int(data.FanDuty);
        
        int deviation = abs(curDutyEC - m_nLastSetDutyEC[i]);
        if (deviation > EC_FAN_DUTY_MAX * EC_TAKEOVER_THRESHOLD / 100)
        {
            needsReclaim[i] = true;
        }
    }

    if (!needsReclaim[0] && !needsReclaim[1])
        return;

    if (!m_takeoverVerification.CanReclaim(GetTickCount64()))
    {
        m_controlVerification = m_takeoverVerification.State();
        ResetFan();
        return;
    }

    m_bEcTakeoverFlag = TRUE;
    ++m_nEcTakeoverCount;
    for (int i = 0; i < 2; ++i)
    {
        if (!needsReclaim[i])
            continue;

        m_pfnSetFanDuty(i + 1, m_nLastSetDutyEC[i]);
        TRACE("EC 接管检测 #%d: 风扇%d 写入=%d 实际=%d，已夺回\n",
            m_nEcTakeoverCount, i, m_nLastSetDutyEC[i], m_nReadbackDuty[i].load());
    }
    m_takeoverVerification.RecordWrite(m_nTargetDuty[0].load(), m_nTargetDuty[1].load(), GetTickCount64());
    m_controlVerification = m_takeoverVerification.State();
}

void CCore::SetFanDuty()
{
    if (m_sensorHealth.IsFaulted() || m_takeoverVerification.IsFaulted() || m_pfnSetFanDuty == NULL ||
        !CanWriteFans(m_startupState.load(), m_userTakeoverAuthorized.load()))
        return;

    int fanCount = 2;
    if (m_pfnGetFANCounter)
        fanCount = min(MAX_EC_FANS, max(1, m_pfnGetFANCounter()));

    int duty;
    for (int i = 0; i < fanCount; i++)
    {
        int targetDuty = (i < 2) ? m_nSetDuty[i] : m_nSetDuty[1];
        
        if (i < 2)
        {
            if (m_nSmoothedDuty[i] < targetDuty)
                m_nSmoothedDuty[i] += SMOOTH_STEP_UP;
            else if (m_nSmoothedDuty[i] > targetDuty)
                m_nSmoothedDuty[i] -= SMOOTH_STEP_DOWN;
            
            if (m_nSmoothedDuty[i] > 100) m_nSmoothedDuty[i] = 100;
            if (m_nSmoothedDuty[i] < 0)   m_nSmoothedDuty[i] = 0;
            
            duty = int(m_nSmoothedDuty[i] * EC_FAN_DUTY_MAX / 100.0 + 0.5);
        }
        else
        {
            duty = int(targetDuty * EC_FAN_DUTY_MAX / 100.0 + 0.5);
        }
        
        m_pfnSetFanDuty(i + 1, duty);
        if (i < 2)
        {
            m_nLastSetDutyEC[i] = duty;
            m_nTargetDuty[i] = m_nSmoothedDuty[i];
        }
    }
    m_bTakeOverStatus = TRUE;
    m_fansTouched = true;
    m_takeoverVerification.RecordWrite(m_nTargetDuty[0].load(), m_nTargetDuty[1].load(), GetTickCount64());
    m_controlVerification = m_takeoverVerification.State();
}

ControlVerification CCore::GetControlVerification() const
{
    if (m_startupState.load() != StartupState::CoreReady)
        return ControlVerification::Fault;
    if (!m_userTakeoverAuthorized.load())
        return ControlVerification::BiosControl;
    return m_controlVerification.load();
}

StartupCheckResult CCore::GetStartupCheckResult() const
{
    std::lock_guard<std::mutex> lock(m_startupCheckMutex);
    return *m_startupCheckResult;
}

BOOL CCore::IsTakeoverAllowedByStartupCheck() const
{
    std::lock_guard<std::mutex> lock(m_startupCheckMutex);
    return m_startupCheckResult->takeoverAllowed ? TRUE : FALSE;
}

void CCore::RecordStartupCheck(int cpuTemperature, int gpuTemperature)
{
    const BOOL gpuAvailable = m_GpuInfo.m_nDeviceID != 0;
    StartupCheckResult result = m_startupSelfCheck.Evaluate(
        m_driverInitialized != FALSE,
        cpuTemperature,
        gpuTemperature,
        m_nCurRPM[0].load(),
        m_nCurRPM[1].load(),
        gpuAvailable != FALSE);
    const BOOL takeoverAllowed = result.takeoverAllowed ? TRUE : FALSE;
    {
        std::lock_guard<std::mutex> lock(m_startupCheckMutex);
        m_startupCheckResult = std::make_shared<StartupCheckResult>(std::move(result));
    }
    if (takeoverAllowed && m_startupState.load() == StartupState::SelfChecking)
    {
        m_startupState = StartupState::CoreReady;
        WriteDiagnosticLog("Startup self-check completed");
        if (m_hWnd) PostMessage(m_hWnd, WM_CORE_INIT_RESULT, 1, 0);
    }
}

void CCore::SetStartupCheckFailure(PCWSTR fault, PCWSTR message)
{
    std::vector<std::wstring> faults{ fault };
    std::lock_guard<std::mutex> lock(m_startupCheckMutex);
    m_startupCheckResult = std::make_shared<StartupCheckResult>(
        std::move(faults), std::vector<std::wstring>{}, 0, false, message);
}

int CCore::GetTargetDuty(int fanIndex) const
{
    return fanIndex >= 0 && fanIndex < 2 ? m_nTargetDuty[fanIndex].load() : 0;
}

int CCore::GetReadbackDuty(int fanIndex) const
{
    return fanIndex >= 0 && fanIndex < 2 ? m_nReadbackDuty[fanIndex].load() : 0;
}

void CCore::SetUserTakeoverAuthorized(BOOL authorized)
{
    if (!authorized)
    {
        m_userTakeoverAuthorized = false;
        m_controlVerification = ControlVerification::BiosControl;
        m_takeoverSessionResetRequested = true;
        m_resetFansRequested = true;
    }
    else
    {
        m_userTakeoverAuthorized = true;
        m_controlVerification = ControlVerification::RequestingTakeover;
        m_takeoverSessionResetRequested = true;
    }
}

void CCore::EnableForcedCooling(BOOL enable)
 {
     if (enable && !CanEnableForcedCooling(m_startupState.load(), m_userTakeoverAuthorized.load()))
     {
         m_bForcedCooling = FALSE;
         return;
     }
     EnterCriticalSection(&m_csConfig);
     m_bForcedCooling = enable;
     if (enable)
     {
         m_nSavedControlMode = m_config.ControlMode;
         m_config.ControlMode = 2;
     }
     else
     {
         m_config.ControlMode = m_nSavedControlMode;
     }
     LeaveCriticalSection(&m_csConfig);
 }

void CCore::SetMaxDutyLimit(int limit)
{
    EnterCriticalSection(&m_csConfig);
    m_config.MaxDutyLimit = max(0, min(100, limit));
    LeaveCriticalSection(&m_csConfig);
    SaveConfigSnapshot();
}

void CCore::SetControlMode(int mode)
{
    EnterCriticalSection(&m_csConfig);
    if (mode == 2)
    {
        m_nSavedControlMode = m_config.ControlMode;
        m_bForcedCooling = TRUE;
    }
    else
    {
        m_bForcedCooling = FALSE;
    }
    m_config.ControlMode = mode;
    LeaveCriticalSection(&m_csConfig);
}

BOOL CCore::CheckTempWarning()
{
    CConfig configSnapshot;
    LockConfig();
    configSnapshot = m_config;
    UnlockConfig();

    const auto decision = m_temperatureAlertPolicy.Evaluate(
        m_nCurTemp[0].load(),
        m_nCurTemp[1].load(),
        configSnapshot.WarningTemp,
        configSnapshot.DesktopNotifications != FALSE,
        GetTickCount64(),
        static_cast<ULONGLONG>(configSnapshot.NotificationCooldownMinutes) * 60 * 1000);
    m_bTempWarning = decision.alertActive;

    if (decision.shouldNotify && m_hWnd)
    {
        NOTIFYICONDATAW nid = { sizeof(nid) };
        nid.hWnd = m_hWnd;
        nid.uID = IDR_MAINFRAME;
        nid.uFlags = NIF_INFO;
        nid.dwInfoFlags = NIIF_WARNING;
        StringCchPrintfW(nid.szInfo, _countof(nid.szInfo),
            L"CPU: %d°C / GPU: %d°C - 温度过高",
            m_nCurTemp[0].load(), m_nCurTemp[1].load());
        StringCchPrintfW(nid.szInfoTitle, _countof(nid.szInfoTitle), L"FanControl Pro 温度告警");
        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }

    return m_bTempWarning;
}

BOOL CCore::IsTemperatureWarning() const
{
    return m_bTempWarning;
}

void CCore::SetWarningSettings(BOOL enabled, int warningTemp, int cooldownMinutes)
{
    LockConfig();
    m_config.DesktopNotifications = !!enabled;
    m_config.WarningTemp = max(60, min(100, warningTemp));
    m_config.NotificationCooldownMinutes = max(1, min(60, cooldownMinutes));
    UnlockConfig();
    SaveConfigSnapshot();
}

void CCore::ApplyPreset(const char* presetName)
{
    EnterCriticalSection(&m_csConfig);
    
    if (strcmp(presetName, "Silent") == 0)
    {
        int silentCurve[TEMP_LEVELS] = {60, 55, 45, 35, 25, 20, 15, 12, 10, 8};
        m_config.MaxDutyLimit = 60;
        m_config.TransitionTemp = 5;
        for (int i = 0; i < TEMP_LEVELS; i++)
        {
            m_config.DutyList[0][i] = silentCurve[i];
            m_config.DutyList[1][i] = silentCurve[i];
        }
    }
    else if (strcmp(presetName, "Performance") == 0)
    {
        m_config.MaxDutyLimit = 100;
        m_config.TransitionTemp = 2;
        int perfCurve[TEMP_LEVELS] = {100, 95, 85, 75, 65, 55, 45, 35, 25, 20};
        for (int i = 0; i < TEMP_LEVELS; i++)
        {
            m_config.DutyList[0][i] = perfCurve[i];
            m_config.DutyList[1][i] = perfCurve[i];
        }
    }
    else if (strcmp(presetName, "Balanced") == 0)
    {
        m_config.LoadDefault();
    }
    
    LeaveCriticalSection(&m_csConfig);
    
    SaveConfigSnapshot();
    m_bForcedRefresh = TRUE;
}
