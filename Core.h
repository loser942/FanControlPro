#pragma once
#include <string>
#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <synchapi.h>
#include "ConfigPersistence.h"
#include "StartupState.h"
#include "ControlVerificationState.h"
#include "TakeoverVerificationPolicy.h"
#include "SensorHealthPolicy.h"
#include "TemperatureAlertPolicy.h"
#include "StartupSelfCheckPolicy.h"
#include "PowerResumePolicy.h"

constexpr UINT WM_CORE_INIT_RESULT = WM_APP + 2;

// ── 编译期常量（替代 #define）──
constexpr int TEMP_LEVELS          = 10;   // 温度档位
constexpr int MAX_FAN_DUTY_LIMIT   = 85;   // 最大转速限制（百分比）
constexpr int FORCED_COOLING_DUTY  = 100;  // 强冷模式转速
constexpr int EC_FAN_DUTY_MAX      = 255;  // EC 风扇负载最大值（0-255）
constexpr int RPM_PULSE_FACTOR     = 2100000; // RPM = 2100000 / 脉冲计数
constexpr int RPM_MIN_PULSE        = 300;  // 有效脉冲最小值
constexpr int RPM_MAX_PULSE        = 5000; // 有效脉冲最大值
constexpr int SMOOTH_STEP_UP       = 4;    // 平滑升速步长
constexpr int SMOOTH_STEP_DOWN     = 2;    // 平滑降速步长
constexpr int EC_REFRESH_TICKS     = 10;   // EC 每秒刷新（10×100ms）
constexpr int MIDNIGHT_GUARD_MS    = 10000; // 午夜回绕保护阈值
constexpr int EC_TAKEOVER_THRESHOLD = 15;    // EC 接管检测阈值（duty偏差>15%视为BIOS接管）
constexpr int MAX_EC_FANS           = 2;     // 本程序只定义了 CPU / GPU 两路映射
constexpr int MAX_VALID_TEMPERATURE  = 120;   // 超出此范围视为 EC 读取异常
constexpr int THERMAL_RECOVERY_GAP  = 3;     // 紧急强冷退出的回差，避免阈值附近反复切换

// 温度阈值（constexpr 数组）
constexpr int TEMP_LIST[TEMP_LEVELS] = { 90, 85, 80, 75, 70, 65, 60, 55, 50, 45 };

// 获取当前时间（6 位数字格式，如 92500 表示 9:25:00）
int GetTime(tm *pt = 0, int offset = 0);
// 计算时间差
int GetTimeInterval(int a, int b, int *p = 0);
// 获取 exe 当前路径
CStringW GetExePath();
void WriteDiagnosticLog(PCSTR message);

// EC 数据结构
struct ECData
{
    byte Remote;    // 温度
    byte Local;
    byte FanDuty;   // 风扇负载，0-255
    byte Reserve;
};

// 温度控制接口函数类型定义
typedef  BOOL(InitIo)(void);                    // 初始化接口
typedef  void(FnSetFanDuty)(int fan_id, int duty); // 设置风扇负载
typedef  int(SetFANDutyAuto)(int fan_id);       // 设置风扇自动
typedef  ECData(GetTempFanDuty)(int fan_id);    // 获取温度状态
typedef  int(GetFANCounter)(void);              // 获取风扇数量
typedef  const char*(GetECVersion)(void);       // 获取版本信息
typedef  int(GetFanRpm)(void);                  // 获取风扇转速

// GPU 控制接口函数类型定义
typedef void(__stdcall In_0_Out_0_Func)(void);
typedef int(__stdcall In_0_Out_n_Func)(void);
typedef int(__stdcall In_1_Out_n_Func)(int);
typedef int(__stdcall In_2_Out_n_Func)(int, int);
typedef PCWSTR(__stdcall In_0_Out_s_Func)(void);

// ── RAII DLL 句柄 ──
class DllHandle
{
public:
    DllHandle() : m_h(nullptr) {}
    explicit DllHandle(HMODULE h) : m_h(h) {}
    ~DllHandle() { Close(); }

    DllHandle(const DllHandle&) = delete;
    DllHandle& operator=(const DllHandle&) = delete;
    DllHandle(DllHandle&& other) noexcept : m_h(other.m_h) { other.m_h = nullptr; }
    DllHandle& operator=(DllHandle&& other) noexcept {
        if (this != &other) { Close(); m_h = other.m_h; other.m_h = nullptr; }
        return *this;
    }

    bool Load(PCWSTR path) { Close(); m_h = LoadLibraryW(path); return m_h != nullptr; }
    void Close() { if (m_h) { FreeLibrary(m_h); m_h = nullptr; } }
    HMODULE Get() const { return m_h; }
    explicit operator bool() const { return m_h != nullptr; }
    FARPROC GetProc(PCSTR name) const { return m_h ? ::GetProcAddress(m_h, name) : nullptr; }

private:
    HMODULE m_h;
};

// GPU 信息类
class CGPUInfo
{
public:
    CGPUInfo();
    ~CGPUInfo();

    int m_nBaseClock;
    int m_nBoostClock;
    PCWSTR m_sName;
    int m_nDeviceID;
    int m_nGraphicsRangeMax;
    int m_nGraphicsRangeMin;
    int m_nMemoryRangeMax;
    int m_nMemoryRangeMin;

    int m_nStandardFrequency;
    int m_nMaxFrequency;

    int m_nGraphicsClock;
    int m_nMemoryClock;
    int m_nUsage;

public:
    BOOL Update();
    BOOL LockFrequency(int frequency = 0);

protected:
    DllHandle m_hGPUdll;
    int m_nLockClock;
    In_0_Out_n_Func *m_pfnInitGPU_API;
    In_1_Out_n_Func *m_pfnSet_GPU_Number;
    In_0_Out_n_Func *m_pfnGet_GPU_Base_Clock;
    In_0_Out_n_Func *m_pfnGet_GPU_Boost_Clock;
    In_0_Out_n_Func *m_pfnCheck_GPU_VRAM_Clock;
    In_0_Out_n_Func *m_pfnGet_GPU_Graphics_Clock;
    In_0_Out_n_Func *m_pfnGet_GPU_Memory_Clock;
    In_0_Out_n_Func *m_pfnGet_Memory_OC_max;
    In_0_Out_n_Func *m_pfnGet_GPU_Util;
    In_0_Out_s_Func *m_pfnGet_GPU_name;
    In_0_Out_n_Func *m_pfnGet_GPU_TotalNumber;
    In_0_Out_n_Func *m_pfnGet_GPU_Overclock_range;
    In_0_Out_n_Func *m_pfnGet_Memory_range;
    In_0_Out_n_Func *m_pfnGet_GPU_Overclock_rangeMax;
    In_0_Out_n_Func *m_pfnGet_GPU_Overclock_rangeMin;
    In_0_Out_n_Func *m_pfnGet_Memory_range_max;
    In_0_Out_n_Func *m_pfnGet_Memory_range_min;
    In_1_Out_n_Func *m_pfnGet_NVDeviceID;
    In_2_Out_n_Func *m_pfnLock_Frequency;
    In_2_Out_n_Func *m_pfnLock_Frequency_MEM;
    In_2_Out_n_Func *m_pfnSet_CoreOC;
    In_2_Out_n_Func *m_pfnSet_MEMOC;
    In_0_Out_0_Func *m_pfnCloseGPU_API;
};

// 配置管理类
class CConfig
{
public:
    CConfig();
    
public:
    int DutyList[2][TEMP_LEVELS];
    int TransitionTemp;
    int UpdateInterval;
    BOOL Linear;
    BOOL TakeOver;
    BOOL AutoTakeoverAfterCheck;
    int ForceTemp;
    int MaxDutyLimit;
    
    BOOL LockGPUFrequency;
    int GPUFrequency;
    
    int ControlMode;
    int ManualDuty[2];

    int WarningTemp;
    BOOL DesktopNotifications;
    int NotificationCooldownMinutes;
    
    wchar_t ConfigPath[MAX_PATH];
    
public:
    void LoadDefault();
    void LoadConfig();
    void SaveConfig();
    void Normalize();
    void ExportConfig(PCWSTR path) const;
    void ImportConfig(PCWSTR path);
};

// 核心控制类
class CCore
{
public:
    CCore();
    ~CCore();
    
protected:
    InitIo          *   m_pfnInitIo;
    FnSetFanDuty    *   m_pfnSetFanDuty;
    SetFANDutyAuto  *   m_pfnSetFANDutyAuto;
    GetTempFanDuty  *   m_pfnGetTempFanDuty;
    GetFANCounter   *   m_pfnGetFANCounter;
    GetECVersion    *   m_pfnGetECVersion;
    GetFanRpm       *   m_pfnGetFANRPM[2];

public:
    std::atomic<BOOL> m_nInit;
    std::atomic<int> m_nExit;
    DllHandle m_hInstDLL;
    CConfig m_config;
    CGPUInfo m_GpuInfo;
    
    std::atomic<int> m_nCurTemp[2];
    int m_nLastTemp[2];
    int m_nSetDuty[2];
    int m_nSetDutyLevel[2];
    std::atomic<int> m_nCurDuty[2];
    std::atomic<int> m_nCurRPM[2];
    
    std::atomic<BOOL> m_bUpdateRPM;
    std::atomic<int> m_nLastUpdateTime;
    std::atomic<BOOL> m_bForcedCooling;
    BOOL m_bTakeOverStatus;
    std::atomic<BOOL> m_bForcedRefresh;
    int m_nNextCheckTime;
    BOOL m_bSetPriority;
    std::atomic<StartupState> m_startupState{ StartupState::UiReady };
    std::atomic_bool m_userTakeoverAuthorized{ false };
    std::atomic_bool m_fansTouched{ false };
    std::atomic_bool m_takeoverSessionResetRequested{ false };
    std::atomic_bool m_resetFansRequested{ false };
    std::atomic_bool m_powerSuspendRequested{ false };
    std::atomic_bool m_powerResumeRequested{ false };
    std::atomic_bool m_manualTakeoverRetryRequested{ false };
    std::atomic_bool m_manualTakeoverPending{ false };
    std::atomic_bool m_ecWritesInhibited{ false };
    ULONGLONG m_resumeSelfCheckNotBeforeTick = 0;
    std::atomic<ControlVerification> m_controlVerification{ ControlVerification::BiosControl };
    std::atomic<int> m_nTargetDuty[2];
    std::atomic<int> m_nReadbackDuty[2];
    std::atomic<DWORD> m_initError{ ERROR_SUCCESS };
    
    int m_nSmoothedDuty[2];
    
    BOOL m_bTempWarning;
    BOOL m_bThermalEmergency;
    TakeoverVerificationPolicy m_takeoverVerification{ EC_TAKEOVER_THRESHOLD, 3000, 2, 5000, 3 };
    SensorHealthPolicy m_sensorHealth;
    PowerResumePolicy m_powerResumePolicy;
    TemperatureAlertPolicy m_temperatureAlertPolicy;
    HWND m_hWnd;

int m_nLastSetDutyEC[2];
     int m_nEcTakeoverCount;
     BOOL m_bEcTakeoverFlag;
     int m_nSavedControlMode;

     HANDLE m_hExitEvent;

 public:
     void SetHWnd(HWND hWnd) { m_hWnd = hWnd; }
     void SignalExit() { if (m_hExitEvent) SetEvent(m_hExitEvent); }
     StartupState GetStartupState() const { return m_startupState.load(); }
     DWORD GetInitError() const { return m_initError.load(); }
    ControlVerification GetControlVerification() const;
    StartupCheckResult GetStartupCheckResult() const;
    BOOL IsTakeoverAllowedByStartupCheck() const;
    BOOL IsEcDllAvailable() const { return m_ecDllAvailable; }
    BOOL IsDriverInitialized() const { return m_driverInitialized; }
     int GetTargetDuty(int fanIndex) const;
     int GetReadbackDuty(int fanIndex) const;
    void SetUserTakeoverAuthorized(BOOL authorized);
    void NotifyPowerSuspend();
    void NotifyPowerResume();
    void SaveConfigSnapshot();

public:
    BOOL Init();
    void Uninit();
    void Run();
    void Work();
    BOOL Update();
    void Control(const CConfig& cfg);
    void CalcLinearDuty(const CConfig& cfg);
    void CalcStdDuty(const CConfig& cfg);
    void CalcManualDuty(const CConfig& cfg);
    void ResetFan();
    void SetFanDuty();
    void VerifyAndReclaim();
    
    void LockConfig()   { EnterCriticalSection(&m_csConfig); }
    void UnlockConfig() { LeaveCriticalSection(&m_csConfig); }

    void EnableForcedCooling(BOOL enable);
    void SetMaxDutyLimit(int limit);
    void SetControlMode(int mode);
    BOOL CheckTempWarning();
    BOOL IsTemperatureWarning() const;
    void SetWarningSettings(BOOL enabled, int warningTemp, int cooldownMinutes);
    void ApplyPreset(const char* presetName);

protected:
    CRITICAL_SECTION m_csConfig;
    std::mutex m_configPersistenceMutex;
    mutable std::mutex m_startupCheckMutex;
    StartupSelfCheckPolicy m_startupSelfCheck;
    std::shared_ptr<const StartupCheckResult> m_startupCheckResult;
    BOOL m_ecDllAvailable = FALSE;
    BOOL m_driverInitialized = FALSE;
    void RecordStartupCheck(int cpuTemperature, int gpuTemperature);
    void SetStartupCheckFailure(PCWSTR fault, PCWSTR message);
    void BeginStartupSelfCheck(BOOL autoTakeoverEnabled, PCSTR logMessage);
};
