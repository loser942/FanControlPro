#include "stdafx.h"
#include "Core.h"
#include <cmath>
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
CString GetExePath()
{
    char pathbuf[1024] = { 0 };
    int pathlen = ::GetModuleFileName(NULL, pathbuf, 1024);
    if (pathlen <= 0)
        return "";

    while (pathlen >= 0)
    {
        if (pathbuf[pathlen--] == '\\')
        {
            break;
        }
    }
    pathbuf[++pathlen] = 0x0;
    CString fname = pathbuf;
    return fname;
}

// ==================== CGPUInfo 实现 ====================

CGPUInfo::CGPUInfo()
{
    CString dllpth = GetExePath() + "\\NVGPU_DLL.dll";
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
    // DllHandle 析构自动 FreeLibrary
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
    CString path = GetExePath() + "\\FanControlPro.cfg";
    strcpy_s(ConfigPath, MAX_PATH, path);
    LoadDefault();
}

void CConfig::LoadDefault()
{
    // 针对 i5-13500H + RTX 4060 优化的默认曲线
    // CPU 风扇曲线（更激进一些，因为 13 代酷睿温度上升快）
    int i = 0;
    DutyList[0][i++] = 85;  // 90°C+ -> 85% (限制在 85%)
    DutyList[0][i++] = 80;  // 85°C+
    DutyList[0][i++] = 70;  // 80°C+
    DutyList[0][i++] = 55;  // 75°C+
    DutyList[0][i++] = 40;  // 70°C+
    DutyList[0][i++] = 30;  // 65°C+
    DutyList[0][i++] = 25;  // 60°C+
    DutyList[0][i++] = 20;  // 55°C+
    DutyList[0][i++] = 15;  // 50°C+
    DutyList[0][i++] = 10;  // 50°C-

    // GPU 风扇曲线（RTX 4060 功耗较低，可以相对保守）
    for (int j = 0; j < TEMP_LEVELS; j++)
        DutyList[1][j] = DutyList[0][j];
    
    // GPU 在低温时可以更低转速
    DutyList[1][8] = 12;  // 50°C+
    DutyList[1][9] = 8;   // 50°C-

    TransitionTemp = 3;        // 3 度迟滞
    UpdateInterval = 2;        // 2 秒更新
    Linear = FALSE;            // 默认阶梯模式
    TakeOver = TRUE;           // 默认接管控制
    ForceTemp = 55;            // 55 度触发强冷
    MaxDutyLimit = MAX_FAN_DUTY_LIMIT; // 85% 最大限制
    
    LockGPUFrequency = FALSE;
    GPUFrequency = 0;
    
    ControlMode = 0;           // 自动模式
    ManualDuty[0] = 50;        // 手动模式默认 50%
    ManualDuty[1] = 50;
}

// 配置格式版本魔数（向前兼容：版本不匹配则回退默认配置）
#define CONFIG_MAGIC 0x46504346  // "FPCF"
#define CONFIG_VERSION 1

void CConfig::LoadConfig()
{
    FILE *fp = fopen(ConfigPath, "rb");
    if (fp == NULL)
    {
        SaveConfig();
        return;
    }

    // 校验版本魔数（向前兼容保护）
    int header[2] = {0};
    if (fread(header, sizeof(header), 1, fp) != 1 ||
        header[0] != CONFIG_MAGIC || header[1] != CONFIG_VERSION)
    {
        fclose(fp);
        LoadDefault();
        SaveConfig();
        return;
    }

    // 跳过 ConfigPath（运行时路径，不序列化到文件）
    const size_t configDataOffset = offsetof(CConfig, ConfigPath) + sizeof(ConfigPath);
    const size_t configDataSize = sizeof(*this) - configDataOffset;

    if (fread(reinterpret_cast<char*>(this) + configDataOffset, configDataSize, 1, fp) != 1)
    {
        fclose(fp);
        LoadDefault();
        SaveConfig();
        return;
    }
    fclose(fp);
}

void CConfig::SaveConfig()
{
    FILE *fp = fopen(ConfigPath, "wb");
    if (fp == NULL)
    {
        AfxMessageBox("无法写入配置文件");
        return;
    }
    // 写入版本魔数（向前兼容）
    int header[2] = { CONFIG_MAGIC, CONFIG_VERSION };
    fwrite(header, sizeof(header), 1, fp);
    // 跳过 ConfigPath（运行时路径，不序列化到文件）
    const size_t configDataOffset = offsetof(CConfig, ConfigPath) + sizeof(ConfigPath);
    const size_t configDataSize = sizeof(*this) - configDataOffset;
    fwrite(reinterpret_cast<char*>(this) + configDataOffset, configDataSize, 1, fp);
    fclose(fp);
}

void CConfig::ExportConfig(PCSTR path) const
{
    FILE *fp = fopen(path, "wb");
    if (fp == NULL)
    {
        AfxMessageBox("无法打开导出路径");
        return;
    }
    int header[2] = { CONFIG_MAGIC, CONFIG_VERSION };
    fwrite(header, sizeof(header), 1, fp);
    const size_t offset = offsetof(CConfig, ConfigPath) + sizeof(ConfigPath);
    const size_t dataSize = sizeof(*this) - offset;
    fwrite(reinterpret_cast<const char*>(this) + offset, dataSize, 1, fp);
    fclose(fp);
}

void CConfig::ImportConfig(PCSTR path)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
    {
        AfxMessageBox("无法打开导入文件，请检查路径");
        return;
    }
    int header[2] = {0};
    if (fread(header, sizeof(header), 1, fp) != 1 ||
        header[0] != CONFIG_MAGIC || header[1] != CONFIG_VERSION)
    {
        fclose(fp);
        AfxMessageBox("配置文件格式不匹配或版本不兼容，导入失败");
        return;
    }
    const size_t offset = offsetof(CConfig, ConfigPath) + sizeof(ConfigPath);
    const size_t dataSize = sizeof(*this) - offset;
    if (fread(reinterpret_cast<char*>(this) + offset, dataSize, 1, fp) != 1)
    {
        AfxMessageBox("配置文件格式不匹配，导入失败");
        LoadDefault();
    }
    fclose(fp);
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
    }
    
    m_bUpdateRPM = FALSE;
    m_nLastUpdateTime = GetTime(0, -5);
    m_bForcedCooling = FALSE;
    m_bTakeOverStatus = FALSE;
    m_bForcedRefresh = FALSE;
    m_nNextCheckTime = 0;
    m_bSetPriority = FALSE;
    
    // 平滑风扇过渡初始值
    m_nSmoothedDuty[0] = 0;
    m_nSmoothedDuty[1] = 0;
    
    // 新增：温度告警
    m_bTempWarning = FALSE;
    m_nWarningTemp = 90;  // 90 度告警
    m_hWnd = NULL;
}

CCore::~CCore()
{
    Uninit();
}

BOOL CCore::Init()
{
    if (m_hInstDLL)
        return TRUE;

    TRACE0("内核开始初始化\n");
    m_nInit = -1;
    
    CString dllpth = GetExePath() + "\\ClevoEcInfo.dll";
    if (!m_hInstDLL.Load(dllpth))
    {
        AfxMessageBox("无法加载 ClevoEcInfo.dll");
        return FALSE;
    }

    HMODULE hDll = m_hInstDLL.Get();
    m_pfnInitIo = (InitIo *)::GetProcAddress(hDll, "InitIo");
    m_pfnSetFanDuty = (SetFanDuty *)::GetProcAddress(hDll, "SetFanDuty");
    m_pfnSetFANDutyAuto = (SetFANDutyAuto *)::GetProcAddress(hDll, "SetFanDutyAuto");
    m_pfnGetTempFanDuty = (GetTempFanDuty *)::GetProcAddress(hDll, "GetTempFanDuty");
    m_pfnGetFANCounter = (GetFANCounter *)::GetProcAddress(hDll, "GetFanCount");
    m_pfnGetECVersion = (GetECVersion *)::GetProcAddress(hDll, "GetECVersion");
    m_pfnGetFANRPM[0] = (GetFanRpm *)::GetProcAddress(hDll, "GetCpuFanRpm");
    m_pfnGetFANRPM[1] = (GetFanRpm *)::GetProcAddress(hDll, "GetGpuFanRpm");

    if (m_pfnInitIo == NULL || m_pfnInitIo() != 1)
    {
        m_hInstDLL.Close();
        AfxMessageBox("EC 接口初始化失败");
        return FALSE;
    }

    TRACE0("内核初始化成功\n");
    
    // ── 崩溃恢复：重置风扇为自动模式 ──
    // 如果上次进程异常退出（崩溃/强杀），EC 可能留在手动转速。
    // 在接管控制前先归还 BIOS，避免上一轮残留状态导致散热异常。
    if (m_pfnSetFANDutyAuto != NULL)
    {
        int fanCount = 2;
        if (m_pfnGetFANCounter)
            fanCount = max(2, m_pfnGetFANCounter());
        for (int i = 0; i < fanCount; i++)
            m_pfnSetFANDutyAuto(i + 1);
    }
    
    m_nInit = 1;
    return TRUE;
}

void CCore::Uninit()
{
    ResetFan();
    m_hInstDLL.Close();  // RAII 自动 FreeLibrary
    m_nInit = 0;
}

void CCore::Run()
{
    m_config.LoadConfig();
    
    if (!m_nInit)
        Init();

    if (m_nInit == 1)
    {
        TRACE0("内核开始运行\n");
        int curtime;
        int ecRefreshTick = 0;  // EC刷新循环计数器（每10次=~1秒）
        while (!m_nExit)
        {
            curtime = GetTime();
            
            // 午夜回绕保护
            if (m_nNextCheckTime > 0 && curtime < m_nNextCheckTime - MIDNIGHT_GUARD_MS)
                m_bForcedRefresh = TRUE;
            
            if (++ecRefreshTick >= EC_REFRESH_TICKS)
            {
                ecRefreshTick = 0;
                if (m_config.TakeOver && m_bTakeOverStatus)
                    SetFanDuty();
            }
            
            if (curtime >= m_nNextCheckTime || m_bForcedRefresh)
            {
                Work();
                m_nLastUpdateTime = curtime;
                m_nNextCheckTime = GetTime(NULL, m_config.UpdateInterval);
                m_bForcedRefresh = FALSE;
                
                if (!m_bSetPriority)
                {
                    m_bSetPriority = TRUE;
                    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
                }
            }
            Sleep(100);
        }
        TRACE0("内核结束运行\n");
        
        // ── 安全退出：恢复风扇自动控制 ──
        // 必须在 Run() 内部调用而非依赖析构函数，
        // 确保在任何退出路径（正常退出/异常退出）都能归还 BIOS 控制权
        ResetFan();
    }
    m_nExit = 2;
}

void CCore::Work()
{
    Update();
    
    // 检查温度告警
    CheckTempWarning();
    
    // 强冷模式处理：一直满转直到用户手动关闭
    if (m_bForcedCooling)
    {
        m_nSetDuty[0] = FORCED_COOLING_DUTY;
        m_nSetDutyLevel[0] = 10;
        m_nSetDuty[1] = FORCED_COOLING_DUTY;
        m_nSetDutyLevel[1] = 10;
        SetFanDuty();
        return;
    }
    
    // 根据控制模式选择控制策略
    if (m_config.TakeOver)
    {
        Control();
    }
    else
    {
        ResetFan();
    }

    // GPU 频率控制
    if (m_config.LockGPUFrequency)
        m_GpuInfo.LockFrequency(m_config.GPUFrequency);
    else
        m_GpuInfo.LockFrequency(0);
}

void CCore::Update()
{
    ECData data;
    int TempErr = 0;
    
    for (int i = 0; i < 2; i++)
    {
        data = m_pfnGetTempFanDuty(i + 1);
        
        // 温度异常检测
        if (abs(data.Remote - this->m_nCurTemp[i]) > 30)
        {
            if (TempErr++ == 0)
            {
                Sleep(1000);
                i--;
                continue;
            }
        }
        
        this->m_nLastTemp[i] = this->m_nCurTemp[i];
        this->m_nCurTemp[i] = data.Remote;
        this->m_nCurDuty[i] = int(data.FanDuty * 100 / double(EC_FAN_DUTY_MAX) + 0.5);

        if (m_bUpdateRPM && m_pfnGetFANRPM[i] != NULL)
        {
            int val = m_pfnGetFANRPM[i]();
            if (val == 0)
                this->m_nCurRPM[i] = 0;
            else if (val > RPM_MIN_PULSE && val < RPM_MAX_PULSE)
                this->m_nCurRPM[i] = RPM_PULSE_FACTOR / val;
            else
                this->m_nCurRPM[i] = -1;
        }
        else
        {
            this->m_nCurRPM[i] = -1;
        }
        TempErr = 0;
    }
    
    if (m_bUpdateRPM)
        m_GpuInfo.Update();
}

void CCore::Control()
{
    switch (m_config.ControlMode)
    {
    case 0: // 自动模式
        if (m_config.Linear)
            CalcLinearDuty();
        else
            CalcStdDuty();
        break;
    case 1: // 手动模式
        CalcManualDuty();
        break;
    case 2: // 强冷模式
        m_nSetDuty[0] = FORCED_COOLING_DUTY;
        m_nSetDuty[1] = FORCED_COOLING_DUTY;
        m_nSetDutyLevel[0] = 10;
        m_nSetDutyLevel[1] = 10;
        break;
    }
    
    SetFanDuty();
}

void CCore::CalcLinearDuty()
{
    int duty, dl;

    for (int i = 0; i < 2; i++)
    {
        // 温度上升时立即响应，下降时需要迟滞
        m_nLastTemp[i] = max(m_nLastTemp[i], m_nCurTemp[i]);
        m_nLastTemp[i] = min(m_nLastTemp[i], m_nCurTemp[i] + m_config.TransitionTemp);

        int j = m_nLastTemp[i];

        if (j < 45)
        {
            duty = m_config.DutyList[i][TEMP_LEVELS - 1];
            dl = 0;
        }
        else if (j >= 90)
        {
            duty = m_config.DutyList[i][0];
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
            int duty_l = m_config.DutyList[i][idx + 1];
            int duty_h = m_config.DutyList[i][idx];
            
            // 线性插值
            duty = int((duty_h - duty_l) / double(temp_h - temp_l) * (j - temp_l) + 0.5) + duty_l;
            dl = TEMP_LEVELS - idx;
        }
        
        // 应用最大转速限制
        duty = min(duty, m_config.MaxDutyLimit);
        
        m_nSetDuty[i] = duty;
        m_nSetDutyLevel[i] = dl;
    }
}

void CCore::CalcStdDuty()
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
            else if (j < TEMP_LIST[k] - m_config.TransitionTemp)
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
        
        // 应用最大转速限制
        int duty = min(m_config.DutyList[i][k], m_config.MaxDutyLimit);
        
        m_nSetDuty[i] = duty;
        m_nSetDutyLevel[i] = dl;
    }
}

void CCore::CalcManualDuty()
{
    for (int i = 0; i < 2; i++)
    {
        // 手动模式直接使用设定的转速，但也要受最大限制约束
        int duty = min(m_config.ManualDuty[i], m_config.MaxDutyLimit);
        m_nSetDuty[i] = duty;
        m_nSetDutyLevel[i] = duty / 10;
    }
}

void CCore::ResetFan()
{
    if (m_bTakeOverStatus && m_pfnSetFANDutyAuto != NULL)
    {
        int fanCount = 2;
        if (m_pfnGetFANCounter)
            fanCount = max(2, m_pfnGetFANCounter());
        for (int i = 0; i < fanCount; i++)
            m_pfnSetFANDutyAuto(i + 1);
        m_bTakeOverStatus = FALSE;
        
        // 重置平滑追踪，下次接管时从0开始平滑过渡
        m_nSmoothedDuty[0] = 0;
        m_nSmoothedDuty[1] = 0;
    }
}

void CCore::SetFanDuty()
{
    if (m_pfnSetFanDuty == NULL)
        return;

    // 获取实际风扇数量（蓝天模具通常 2-3 个）
    int fanCount = 2;
    if (m_pfnGetFANCounter)
        fanCount = max(2, m_pfnGetFANCounter());

    int duty;
    for (int i = 0; i < fanCount; i++)
    {
        int targetDuty = (i < 2) ? m_nSetDuty[i] : m_nSetDuty[1]; // 第3+个风扇跟随GPU
        
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
    }
    m_bTakeOverStatus = TRUE;
}

// ==================== 新增功能实现 ====================

void CCore::EnableForcedCooling(BOOL enable)
{
    m_bForcedCooling = enable;
    if (enable)
    {
        m_config.ControlMode = 2;
    }
}

void CCore::SetMaxDutyLimit(int limit)
{
    m_config.MaxDutyLimit = max(0, min(100, limit));
    m_config.SaveConfig();
}

void CCore::SetControlMode(int mode)
{
    m_config.ControlMode = mode;
    if (mode == 2)
    {
        m_bForcedCooling = TRUE;
    }
    else
    {
        m_bForcedCooling = FALSE;
    }
}

BOOL CCore::CheckTempWarning()
{
    if (m_nCurTemp[0] >= m_nWarningTemp || m_nCurTemp[1] >= m_nWarningTemp)
    {
        if (!m_bTempWarning)
        {
            m_bTempWarning = TRUE;
            // 托盘气泡告警
            if (m_hWnd)
            {
                NOTIFYICONDATA nid = { sizeof(nid) };
                nid.hWnd = m_hWnd;
                nid.uID = IDR_MAINFRAME;  // 使用主托盘图标ID（已注册），非独立ID
                nid.uFlags = NIF_INFO;
                nid.dwInfoFlags = NIIF_WARNING;
                sprintf_s(nid.szInfo, 256, "CPU: %d°C / GPU: %d°C - 温度过高！", 
                    m_nCurTemp[0], m_nCurTemp[1]);
                strcpy_s(nid.szInfoTitle, 64, "FanControl Pro 温度告警");
                Shell_NotifyIcon(NIM_MODIFY, &nid);
            }
        }
        return TRUE;
    }
    else
    {
        m_bTempWarning = FALSE;
        return FALSE;
    }
}

void CCore::ApplyPreset(const char* presetName)
{
    if (strcmp(presetName, "Silent") == 0)
    {
        // 静音模式预设（使用绝对曲线，避免重复应用累积退化）
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
        // 性能模式预设
        m_config.MaxDutyLimit = 100;
        m_config.TransitionTemp = 2;
        // 更激进的曲线
        int perfCurve[TEMP_LEVELS] = {100, 95, 85, 75, 65, 55, 45, 35, 25, 20};
        for (int i = 0; i < TEMP_LEVELS; i++)
        {
            m_config.DutyList[0][i] = perfCurve[i];
            m_config.DutyList[1][i] = perfCurve[i];
        }
    }
    else if (strcmp(presetName, "Balanced") == 0)
    {
        // 平衡模式（默认）
        LoadDefault();
    }
    
    m_config.SaveConfig();
    m_bForcedRefresh = TRUE;
}
