# FanControl Pro Win11 Unicode UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在不改变 x86 EC DLL 兼容边界和启动接管安全规则的前提下，修复中文乱码，交付紧凑的 Win11 双栏界面，并消除重复温度通知。

**Architecture:** 将温度告警决策抽为不依赖 MFC 的 `TemperatureAlertPolicy`，用纯 C++17 测试其回差和冷却逻辑；`CCore` 保留通知执行和配置持久化职责。项目切换为 Unicode，MFC UI 只使用宽字符 API；加载 DLL、导出函数、二进制配置与日志则保留显式 ANSI 调用。对话框资源提供紧凑双栏基础布局，`CFanControlProDlg` 管理高级区域可见性和控件启用状态。

**Tech Stack:** C++17、MFC 静态链接、Win32 API、Visual Studio 2022 v143、MSBuild、Win32、x86 `ClevoEcInfo.dll`。

## Global Constraints

- 只构建 `Debug|Win32` 与 `Release|Win32`，不得恢复 x64 配置。
- 启动后保持 BIOS 控制；只有 `CoreReady && userAuthorized` 才能写入风扇。
- `ClevoEcInfo.dll`、GPU DLL、`GetProcAddress`、二进制配置和诊断日志保留显式 ANSI 边界。
- 所有用户可见文字、资源、主窗口与托盘通知必须使用 Unicode 中文文本。
- 默认阈值为 90 C，回差为 5 C，默认通知冷却时间为 10 分钟；阈值可配范围 60-100 C，冷却时间范围 1-60 分钟。
- 未启用桌面通知时不得调用 Shell 通知，但界面仍必须显示温度告警状态。
- 对 `sources/` 下文件只读。

---

## 文件职责

- `TemperatureAlertPolicy.h`：纯 C++17 告警状态机接口和默认值。
- `diagnostics/temperature_alert_policy_test.cpp`：不依赖 MFC 的告警状态机断言测试。
- `Core.h` / `Core.cpp`：配置字段、告警状态机集成、Unicode 托盘通知及 ANSI 硬件边界。
- `FanControlPro.vcxproj`：在 UI 全面改为宽字符后切换 MFC 应用 Unicode 字符集，并列出新增头文件。
- `FanControlProDlg.h` / `FanControlProDlg.cpp`：宽字符 UI 文本、告警控件、紧凑/高级布局状态和输入校验。
- `resource.h` / `FanControlPro.rc`：控件 ID 与 Win11 紧凑双栏资源模板。
- `diagnostics/startup_state_test.cpp`：完整验证启动阶段禁止风扇写入的状态契约。

### Task 1: 告警状态机与安全状态测试

**Files:**
- Create: `TemperatureAlertPolicy.h`
- Create: `diagnostics/temperature_alert_policy_test.cpp`
- Modify: `diagnostics/startup_state_test.cpp`

**Interfaces:**
- Produces: `struct TemperatureAlertDecision { bool alertActive; bool shouldNotify; };`
- Produces: `class TemperatureAlertPolicy { TemperatureAlertDecision Evaluate(int cpuTemp, int gpuTemp, int threshold, bool notificationsEnabled, unsigned long long nowMs, unsigned long long cooldownMs); }`
- Consumes: `StartupState`, `CanEnableTakeover`, `CanWriteFans`。

- [ ] **Step 1: 写入会失败的告警测试与完整状态契约**

```cpp
#include "../TemperatureAlertPolicy.h"
#include <cassert>

int main()
{
    TemperatureAlertPolicy policy;
    assert(!policy.Evaluate(89, 88, 90, true, 0, 600000).alertActive);
    assert(policy.Evaluate(90, 70, 90, true, 1, 600000).shouldNotify);
    assert(!policy.Evaluate(95, 70, 90, true, 2, 600000).shouldNotify);
    assert(policy.Evaluate(84, 84, 90, true, 3, 600000).alertActive == false);
    assert(policy.Evaluate(92, 70, 90, true, 600004, 600000).shouldNotify);
    assert(!policy.Evaluate(95, 70, 90, false, 600005, 600000).shouldNotify);
}
```

在 `startup_state_test.cpp` 保留已有断言，并确保以下四条存在：

```cpp
assert(!CanWriteFans(StartupState::UiReady, true));
assert(!CanWriteFans(StartupState::CoreStarting, true));
assert(!CanWriteFans(StartupState::CoreFailed, true));
assert(!CanWriteFans(StartupState::Exiting, true));
assert(!CanEnableTakeover(StartupState::Exiting));
```

- [ ] **Step 2: 运行测试，确认告警测试因缺少头文件失败**

Run: `cl /nologo /std:c++17 /EHsc diagnostics\temperature_alert_policy_test.cpp /Fe:diagnostics\temperature_alert_policy_test.exe`

Expected: 编译失败，提示找不到 `TemperatureAlertPolicy.h`。

- [ ] **Step 3: 实现最小状态机**

```cpp
#pragma once

struct TemperatureAlertDecision { bool alertActive; bool shouldNotify; };

class TemperatureAlertPolicy {
public:
    TemperatureAlertDecision Evaluate(int cpu, int gpu, int threshold, bool enabled,
        unsigned long long now, unsigned long long cooldown)
    {
        const bool above = cpu >= threshold || gpu >= threshold;
        const bool rearmed = cpu < threshold - 5 && gpu < threshold - 5;
        if (rearmed) m_armed = true;
        const bool entering = above && m_armed;
        if (entering) m_armed = false;
        const bool canNotify = entering && enabled && (m_lastNotificationMs == 0 || now - m_lastNotificationMs >= cooldown);
        if (canNotify) m_lastNotificationMs = now;
        return { above || !m_armed, canNotify };
    }
private:
    bool m_armed = true;
    unsigned long long m_lastNotificationMs = 0;
};
```

- [ ] **Step 4: 重新编译并运行两个独立测试**

Run: `cl /nologo /std:c++17 /EHsc diagnostics\temperature_alert_policy_test.cpp /Fe:diagnostics\temperature_alert_policy_test.exe && diagnostics\temperature_alert_policy_test.exe && cl /nologo /std:c++17 /EHsc diagnostics\startup_state_test.cpp /Fe:diagnostics\startup_state_test.exe && diagnostics\startup_state_test.exe`

Expected: 两个进程退出码均为 0。

- [ ] **Step 5: 提交状态机与测试**

```powershell
git add TemperatureAlertPolicy.h diagnostics/temperature_alert_policy_test.cpp diagnostics/startup_state_test.cpp
git commit -m "feat: add throttled temperature alert policy"
```

### Task 2: 核心配置、Unicode 通知与 ANSI 边界

**Files:**
- Modify: `Core.h`
- Modify: `Core.cpp`

**Interfaces:**
- Consumes: `TemperatureAlertPolicy::Evaluate`。
- Produces: `CConfig::WarningTemp`、`CConfig::DesktopNotifications`、`CConfig::NotificationCooldownMinutes`。
- Produces: `CCore::IsTemperatureWarning() const` 和 `CCore::SetWarningSettings(BOOL, int, int)`。

- [ ] **Step 1: 在核心测试旁新增编译期接口使用点**

在 `Core.cpp` 的 `CheckTempWarning()` 中临时包含以下调用；此时尚未声明相应字段与方法，完整工程应编译失败：

```cpp
const auto decision = m_temperatureAlertPolicy.Evaluate(
    m_nCurTemp[0].load(), m_nCurTemp[1].load(), m_config.WarningTemp,
    m_config.DesktopNotifications != FALSE, GetTickCount64(),
    static_cast<unsigned long long>(m_config.NotificationCooldownMinutes) * 60 * 1000);
```

- [ ] **Step 2: 构建 Win32 Debug，确认缺失成员导致失败**

Run: `& 'E:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' FanControlPro.sln /t:Build /p:Configuration=Debug /p:Platform=Win32 /m`

Expected: 编译失败，错误指向缺少的告警策略或配置成员。

- [ ] **Step 3: 实现配置与 Unicode 通知**

在 `CConfig::LoadDefault()` 设定：

```cpp
WarningTemp = 90;
DesktopNotifications = TRUE;
NotificationCooldownMinutes = 10;
```

在 `Normalize()` 钳制：

```cpp
WarningTemp = max(60, min(100, WarningTemp));
DesktopNotifications = !!DesktopNotifications;
NotificationCooldownMinutes = max(1, min(60, NotificationCooldownMinutes));
```

将 `CONFIG_VERSION` 增加到 `3`；旧配置版本或读取失败时调用 `LoadDefault()` 和 `SaveConfig()`，使新增字段具备安全默认值。添加 `TemperatureAlertPolicy m_temperatureAlertPolicy;`，并改写 `CheckTempWarning()`：以决策结果更新 `m_bTempWarning`，仅在 `shouldNotify && m_hWnd` 时使用 `NOTIFYICONDATAW`、`StringCchPrintfW` 与 `Shell_NotifyIconW(NIM_MODIFY, &nid)`。通知正文格式为 `L"CPU: %d°C / GPU: %d°C - 温度过高"`，标题为 `L"FanControl Pro 温度告警"`。

将两个配置方法实现为在配置临界区内更新、范围校验、释放锁后 `SaveConfig()`；`IsTemperatureWarning()` 返回 `m_bTempWarning != FALSE`。

在 `ClInclude` 中加入 `TemperatureAlertPolicy.h`。项目字符集切换由 Task 3 与全部 UI 宽字符改造一起完成，避免此任务把已知的旧 UI 窄字符错误带入中间构建。

保留 `DllHandle::Load(PCSTR)`、`DllHandle::GetProc(PCSTR)`、`LoadLibraryA`、`GetProcAddress`、`ConfigPath`、`fopen` 与 `WriteDiagnosticLog(PCSTR)`。修复 `CString` 到 `char[]` 的现有路径复制时使用明确的 `CStringA(CW2A(path, CP_ACP))`，禁止依赖 Unicode 下的隐式转换。

- [ ] **Step 4: 构建 Debug 与 Release 并运行独立告警测试**

Run: `& 'E:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' FanControlPro.sln /t:Build /p:Configuration=Debug /p:Platform=Win32 /m; & 'E:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' FanControlPro.sln /t:Build /p:Configuration=Release /p:Platform=Win32 /m; diagnostics\temperature_alert_policy_test.exe`

Expected: 两个 MSBuild 命令退出码为 0，告警测试退出码为 0。

- [ ] **Step 5: 提交核心告警与字符集迁移**

```powershell
git add Core.h Core.cpp TemperatureAlertPolicy.h
git commit -m "fix: use Unicode throttled temperature notifications"
```

### Task 3: 紧凑双栏资源与 Unicode 界面逻辑

**Files:**
- Modify: `resource.h`
- Modify: `FanControlPro.rc`
- Modify: `FanControlProDlg.h`
- Modify: `FanControlProDlg.cpp`
- Modify: `FanControlPro.vcxproj`

**Interfaces:**
- Consumes: `CCore::IsTemperatureWarning()`、`CCore::SetWarningSettings(BOOL, int, int)`。
- Produces: `IDC_STATIC_ADVANCED_GROUP`、`IDC_CHECK_DESKTOP_NOTIFICATIONS`、`IDC_EDIT_WARNING_TEMP`、`IDC_EDIT_NOTIFICATION_COOLDOWN` 和 `CFanControlProDlg::ApplyResponsiveLayout()`。

- [ ] **Step 1: 声明新的资源控件和布局方法，确认旧实现不能通过链接**

在 `resource.h` 定义连续的控件 ID：

```cpp
#define IDC_STATIC_ADVANCED_GROUP 1033
#define IDC_CHECK_DESKTOP_NOTIFICATIONS 1034
#define IDC_EDIT_WARNING_TEMP 1035
#define IDC_EDIT_NOTIFICATION_COOLDOWN 1036
```

在 `FanControlProDlg.h` 声明 `CButton m_ctlDesktopNotifications;`、两个 `CEdit`、`void ApplyResponsiveLayout();`、`void UpdateWarningStatus();` 和 `afx_msg void OnSize(UINT, int, int);`，但先不实现方法。在 `OnInitDialog()` 末尾加入 `ApplyResponsiveLayout();`，并加入 `ON_WM_SIZE()` 消息映射，使新方法有实际调用点。

- [ ] **Step 2: 构建 Debug，确认未定义的新 UI 方法失败**

Run: `& 'E:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' FanControlPro.sln /t:Build /p:Configuration=Debug /p:Platform=Win32 /m`

Expected: 链接失败，提示 `ApplyResponsiveLayout` 或 `UpdateWarningStatus` 未定义。

- [ ] **Step 3: 实现资源模板和 UI 逻辑**

将主对话框改为约 `470 x 300` DLU 的默认尺寸：左侧“运行状态”栏放 CPU/GPU 温度、进度条、RPM、使用率和启动状态；右侧“风扇控制”栏放模式、接管、强冷、两路滑块和数值输入。底部放“加载配置”“保存配置”“重置默认”和“高级设置”。高级设置区域放在默认区域下方，组框使用 `IDC_STATIC_ADVANCED_GROUP`，初始隐藏，包含原有高级控件及新增“启用桌面通知”“告警温度(°C)”“最短通知间隔(分钟)”。所有资源文字使用中文与 `Segoe UI`。

`DoDataExchange` 绑定新增控件；`OnInitDialog` 用 `L"自动模式"`、`L"手动模式"`、`L"强冷模式"` 添加下拉项；所有 `SetWindowTextA`、`GetWindowTextA`、窄 `sprintf_s` UI 格式化、窄 `AfxMessageBox` 和 `MessageBox` 替换为 Unicode 版本。通过 `CStringW::Format(L"CPU: %d°C", temp)` 等格式化实时文字。随后将两处 `<CharacterSet>MultiByte</CharacterSet>` 替换为 `<CharacterSet>Unicode</CharacterSet>`，使全工程只在 UI 已转换后切换字符集。

`UpdateGui()` 从配置读取告警字段，回填新增控件，调用 `UpdateWarningStatus()`；该方法在 `m_core.IsTemperatureWarning()` 为真时显示 `L"温度告警：请检查散热状态"`，否则保持启动状态。`CheckAndSave()` 读取三个新增输入，分别校验 60-100、1-60，失败时使用中文提示并将焦点放到对应控件；校验成功后调用 `SetWarningSettings()`。

`SetAdvancedMode()` 通过 `ShowWindow(SW_SHOW/SW_HIDE)` 显隐 `IDC_STATIC_ADVANCED_GROUP` 及全部高级设置控件，切换按钮文字为 `L"收起高级设置"` / `L"高级设置"`，并调用 `ApplyResponsiveLayout()`。`OnSize()` 调用 `CDialogEx::OnSize()` 后调用该方法；它在 `WM_SIZE` 和切换后根据当前客户区宽度计算左右栏宽度，设置不低于 440x280 DLU 等效的最小窗口尺寸，重新放置底部按钮，避免文本重叠。

- [ ] **Step 4: 构建 Debug 与 Release，并做静态 Unicode 检查**

Run: `& 'E:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' FanControlPro.sln /t:Build /p:Configuration=Debug /p:Platform=Win32 /m; & 'E:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' FanControlPro.sln /t:Build /p:Configuration=Release /p:Platform=Win32 /m; Select-String -Path FanControlProDlg.cpp -Pattern 'SetWindowTextA|GetWindowTextA|Shell_NotifyIcon\('`

Expected: 两个构建成功；静态检查没有命中 UI 宽字符禁用 API。

- [ ] **Step 5: 提交资源和界面改造**

```powershell
git add resource.h FanControlPro.rc FanControlProDlg.h FanControlProDlg.cpp FanControlPro.vcxproj
git commit -m "feat: redesign compact Unicode Win11 interface"
```

### Task 4: 发布前验证与本机部署

**Files:**
- Modify: `D:\Download\Fans\FanControlPro.exe`（部署产物）
- Create: `D:\Download\Fans\FanControlPro.before-win11-ui.exe`（旧版本备份）

**Interfaces:**
- Consumes: `Release|Win32` 产物、现有 `D:\Download\Fans\ClevoEcInfo.dll`。
- Produces: 可启动的目标电脑测试版本及 `FanControlPro.debug.log` 运行证据。

- [ ] **Step 1: 运行全部独立逻辑测试和干净 Release 构建**

Run: `diagnostics\startup_state_test.exe; diagnostics\temperature_alert_policy_test.exe; & 'E:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' FanControlPro.sln /t:Rebuild /p:Configuration=Release /p:Platform=Win32 /m`

Expected: 测试退出码均为 0，Release Rebuild 退出码为 0。

- [ ] **Step 2: 备份旧 EXE 并复制 Release 产物**

```powershell
Copy-Item -LiteralPath 'D:\Download\Fans\FanControlPro.exe' -Destination 'D:\Download\Fans\FanControlPro.before-win11-ui.exe' -Force
Copy-Item -LiteralPath '.\Release\FanControlPro.exe' -Destination 'D:\Download\Fans\FanControlPro.exe' -Force
```

- [ ] **Step 3: 在目标目录启动并检查日志与进程响应**

Run: `Start-Process -FilePath 'D:\Download\Fans\FanControlPro.exe'; Start-Sleep -Seconds 5; Get-Process FanControlPro; Get-Content -Tail 30 'D:\Download\Fans\FanControlPro.debug.log'`

Expected: 进程存在且响应；日志包含 `ClevoEcInfo.dll loaded`、`EC exports=1 InitIo=1` 和 `Core.Init completed read-only probe`。测试过程中不勾选“接管控制”。

- [ ] **Step 4: 人工确认显示与告警行为**

确认主窗口中文、`°C`、模式下拉项和启动状态正常；将“启用桌面通知”关闭后不再出现右下角通知，界面告警状态仍可见。通知启用时，连续高温读取只显示一次，必须在 CPU/GPU 均低于阈值 5 C 后再次越阈值才可能通知。

- [ ] **Step 5: 提交部署无关源码验证结果**

```powershell
git status --short
git log --oneline -4
```

Expected: 源码提交完整；不将 `D:\Download\Fans` 二进制或本地 `.superpowers/` 元数据加入 Git。
