#pragma once
using namespace std;
#include <string>
#include <algorithm>
#include <atomic>

// 最大转速限制（百分比）
#define MAX_FAN_DUTY_LIMIT 85
// 强冷模式转速
#define FORCED_COOLING_DUTY 100
// 温度档位（10 档）
#define TEMP_LEVELS 10
// 默认温度阈值数组
#define DEFAULT_TEMP_THRESHOLDS {90, 85, 80, 75, 70, 65, 60, 55, 50, 45}

// 获取当前时间（6 位数字格式，如 92500 表示 9:25:00）
int GetTime(tm *pt = 0, int offset = 0);
// 计算时间差
int GetTimeInterval(int a, int b, int *p = 0);
// 获取 exe 当前路径
CString GetExePath();

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
typedef  void(SetFanDuty)(int fan_id, int duty); // 设置风扇负载
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

    int m_nStandardFrequency; // 默认频率
    int m_nMaxFrequency;      // 最大频率

    int m_nGraphicsClock;
    int m_nMemoryClock;
    int m_nUsage; // 使用率%

public:
    BOOL Update(); // 更新 GPU 频率和使用率
    BOOL LockFrequency(int frequency = 0); // 锁定频率

protected:
    HMODULE m_hGPUdll;
    int m_nLockClock;
    // 接口函数指针
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
    // 风扇曲线配置 [2 个风扇][10 个温度档位]
    int DutyList[2][TEMP_LEVELS];
    int TransitionTemp;      // 过渡温度（迟滞）
    int UpdateInterval;      // 更新间隔（秒）
    BOOL Linear;             // 线性模式
    BOOL TakeOver;           // 接管控制
    int ForceTemp;           // 强冷触发温度
    int MaxDutyLimit;        // 最大转速限制（默认 85%）
    
    // GPU 控制
    BOOL LockGPUFrequency;
    int GPUFrequency;
    
    // 新增：模式配置
    int ControlMode;         // 0=自动，1=手动，2=强冷
    int ManualDuty[2];       // 手动模式转速设置 [CPU, GPU]
    
    // 配置文件路径（固定数组避免二进制序列化崩溃）
    char ConfigPath[MAX_PATH];
    
public:
    void LoadDefault();      // 加载默认配置
    void LoadConfig();       // 读取配置
    void SaveConfig();       // 保存配置
    void ExportConfig(PCSTR path) const; // 导出配置
    void ImportConfig(PCSTR path);       // 导入配置
};

// 核心控制类
class CCore
{
public:
    CCore();
    ~CCore();
    
protected:
    // EC 接口函数指针
    InitIo          *   m_pfnInitIo;
    SetFanDuty      *   m_pfnSetFanDuty;
    SetFANDutyAuto  *   m_pfnSetFANDutyAuto;
    GetTempFanDuty  *   m_pfnGetTempFanDuty;
    GetFANCounter   *   m_pfnGetFANCounter;
    GetECVersion    *   m_pfnGetECVersion;
    GetFanRpm       *   m_pfnGetFANRPM[2];

public:
    std::atomic<BOOL> m_nInit;    // 初始化状态（原子操作，跨线程安全）
    std::atomic<int> m_nExit;     // 退出信号（原子操作，跨线程安全）
    HINSTANCE m_hInstDLL;      // DLL 模块句柄
    CConfig m_config;          // 配置
    CGPUInfo m_GpuInfo;        // GPU 信息
    
    // 温度与风扇状态
    int m_nCurTemp[2];         // 当前温度 [CPU, GPU]
    int m_nLastTemp[2];        // 上次温度
    int m_nSetDuty[2];         // 设置负载
    int m_nSetDutyLevel[2];    // 设置档位
    int m_nCurDuty[2];         // 当前负载
    int m_nCurRPM[2];          // 当前转速
    
    // 控制状态
    BOOL m_bUpdateRPM;         // 是否更新 RPM
    int m_nLastUpdateTime;     // 最后更新时间
    BOOL m_bForcedCooling;     // 强冷模式
    BOOL m_bTakeOverStatus;    // 接管状态
    BOOL m_bForcedRefresh;     // 强制刷新
    int m_nNextCheckTime;      // 下次检查时间（替代 static 局部变量）
    BOOL m_bSetPriority;       // 是否已设置进程高优先级（替代 static 局部变量）
    
    // 平滑风扇过渡
    int m_nSmoothedDuty[2];    // 当前平滑后的风扇转速 [CPU, GPU]
    
    // 新增：温度告警
    BOOL m_bTempWarning;       // 温度告警状态
    int m_nWarningTemp;        // 告警温度阈值
    HWND m_hWnd;               // 主窗口句柄（用于托盘通知）

public:
    void SetHWnd(HWND hWnd) { m_hWnd = hWnd; }
    
public:
    BOOL Init();               // 初始化
    void Uninit();             // 反初始化
    void Run();                // 主循环
    void Work();               // 工作函数
    void Update();             // 更新状态
    void Control();            // 控制风扇
    void CalcLinearDuty();     // 计算线性转速
    void CalcStdDuty();        // 计算标准转速
    void CalcManualDuty();     // 计算手动转速
    void ResetFan();           // 重置风扇
    void SetFanDuty();         // 设置风扇负载
    
    // 新增功能
    void EnableForcedCooling(BOOL enable);  // 启用强冷
    void SetMaxDutyLimit(int limit);        // 设置最大转速限制
    void SetControlMode(int mode);          // 设置控制模式
    BOOL CheckTempWarning();                // 检查温度告警
    void ApplyPreset(const char* presetName);     // 应用预设配置
};
