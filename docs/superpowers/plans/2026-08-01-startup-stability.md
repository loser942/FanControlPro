# FanControlPro 启动稳定性实现计划

> **面向执行代理：** 必须使用 `superpowers:subagent-driven-development`（推荐）或 `superpowers:executing-plans` 逐任务执行。本计划使用 `- [ ]` 复选框跟踪步骤。

**目标：** 让 Win32 版本先显示可诊断的主窗口，再异步初始化 EC；启动失败时始终保持 BIOS 风扇控制，且不再显示正文为空的错误消息框。

**架构：** `OnInitDialog` 只创建可见界面、显示初始状态并投递延后启动消息。核心线程通过 `WM_APP` 消息把初始化结果交回 UI 线程；UI 负责状态文字和控件可用性，`CCore` 只负责只读探测与明确授权后的风扇写入。所有启动边界写入 ASCII 日志。

**技术栈：** Visual C++ / MFC、Win32 消息循环、C++17、x86 `ClevoEcInfo.dll`。

## 全局约束

- 只构建并验证 `Release|Win32`；`ClevoEcInfo.dll` 必须与 EXE 同目录。
- 启动期间不得调用 `SetFanDuty` 或 `SetFanDutyAuto`；仅用户明确勾选“接管控制”后才能执行风扇写入。
- 所有启动日志只使用 ASCII 文本，并在每个 Win32 边界记录 `GetLastError()`。
- 后台线程不得调用 `AfxMessageBox`、更新 MFC 控件或显示其他模态窗口。
- GPU 频率锁定默认禁用，且所有开机自启/托盘检查都不得阻塞主窗口首次显示。

---

### 任务 1：定义并测试启动状态转换

**文件：**
- 新建：`StartupState.h`
- 新建：`diagnostics/startup_state_test.cpp`

**接口：**
- 产出：`enum class StartupState { UiReady, CoreStarting, CoreReady, CoreFailed, Exiting };`
- 产出：`bool CanEnableTakeover(StartupState state);`
- 产出：`bool CanWriteFans(StartupState state, bool userAuthorized);`

- [ ] **步骤 1：编写失败测试**

在 `diagnostics/startup_state_test.cpp` 写入以下断言，引用尚不存在的 `StartupState.h`：

```cpp
#include "../StartupState.h"
#include <cassert>

int main()
{
    assert(!CanEnableTakeover(StartupState::UiReady));
    assert(!CanEnableTakeover(StartupState::CoreStarting));
    assert(CanEnableTakeover(StartupState::CoreReady));
    assert(!CanEnableTakeover(StartupState::CoreFailed));
    assert(!CanWriteFans(StartupState::CoreReady, false));
    assert(CanWriteFans(StartupState::CoreReady, true));
    return 0;
}
```

- [ ] **步骤 2：验证测试失败**

运行：

```powershell
cl /nologo /EHsc /std:c++17 /Fe:diagnostics\startup_state_test.exe diagnostics\startup_state_test.cpp
```

预期：因缺少 `StartupState.h` 而失败。

- [ ] **步骤 3：实现最小状态契约**

新建 `StartupState.h`，保持其不依赖 MFC：

```cpp
#pragma once

enum class StartupState { UiReady, CoreStarting, CoreReady, CoreFailed, Exiting };

inline bool CanEnableTakeover(StartupState state)
{
    return state == StartupState::CoreReady;
}

inline bool CanWriteFans(StartupState state, bool userAuthorized)
{
    return state == StartupState::CoreReady && userAuthorized;
}
```

- [ ] **步骤 4：验证状态契约**

运行：

```powershell
cl /nologo /EHsc /std:c++17 /Fe:diagnostics\startup_state_test.exe diagnostics\startup_state_test.cpp
& .\diagnostics\startup_state_test.exe
```

预期：编译成功，进程退出码为 `0`。

- [ ] **步骤 5：提交本任务**

```powershell
git add StartupState.h diagnostics/startup_state_test.cpp
git commit -m "test: define startup safety state contract"
```

### 任务 2：让窗口先显示，并记录完整 UI 启动边界

**文件：**
- 修改：`FanControlProDlg.h`
- 修改：`FanControlProDlg.cpp`
- 修改：`resource.h`
- 修改：`FanControlPro.rc`

**接口：**
- 消耗：`StartupState`、`CanEnableTakeover`。
- 产出：`WM_DEFERRED_STARTUP` 消息处理器、`SetStartupStatus(PCSTR)`、`SetTakeoverControlsEnabled(BOOL)`。

- [ ] **步骤 1：为 UI 边界准备诊断检查**

在 `OnInitDialog` 的每个同步边界后记录以下固定日志文本，并保持托盘、开机自启和核心线程尚未启动：

```cpp
WriteDiagnosticLog("UI system menu ready");
WriteDiagnosticLog("UI controls ready");
WriteDiagnosticLog("UI visible startup posted");
```

手工启动当前程序并读取 `FanControlPro.debug.log`。预期此阶段还无法出现第三条记录，证明测试能区分当前问题。

- [ ] **步骤 2：在窗口创建完成后延后启动工作**

定义 `WM_DEFERRED_STARTUP` 为 `WM_APP + 1`（与托盘的 `WM_USER + 1` 分离），在消息映射中注册：

```cpp
ON_MESSAGE(WM_DEFERRED_STARTUP, &CFanControlProDlg::OnDeferredStartup)
```

`OnInitDialog` 只完成控件初始值、调用 `SetStartupStatus("正在初始化监控，BIOS 风扇控制保持启用")`、禁用接管相关控件，然后调用 `PostMessage(WM_DEFERRED_STARTUP)` 并返回 `TRUE`。不得在此函数中调用 `SetTray`、`SetAutorunReg`、`SetAutorunTask` 或 `CreateThread`。

- [ ] **步骤 3：实现状态区域与控件闸门**

增加静态文本控件 `IDC_STATIC_STARTUP_STATUS`，并实现：

```cpp
void CFanControlProDlg::SetTakeoverControlsEnabled(BOOL enabled)
{
    m_ctlTakeOver.EnableWindow(enabled);
    m_ctlForcedCooling.EnableWindow(enabled);
    m_ctlMode.EnableWindow(enabled);
}
```

每次状态变化更新 `StartupState`、状态文本和 ASCII 日志。任何 `Shell_NotifyIcon` 返回 `FALSE` 时仅记录失败并继续；不得改变窗口可见性。

- [ ] **步骤 4：验证 UI 路径**

运行 `Release\FanControlPro.exe`，在程序首次显示后读取日志。预期顺序至少包含：`Dialog initialization started`、`Single-instance mutex acquired`、`UI system menu ready`、`UI controls ready`、`UI visible startup posted`。窗口必须在任何 EC/托盘/开机自启日志之前可见。

- [ ] **步骤 5：提交本任务**

```powershell
git add FanControlProDlg.h FanControlProDlg.cpp resource.h FanControlPro.rc
git commit -m "fix: show startup status before background initialization"
```

### 任务 3：隔离后台初始化失败，并维持只读探测

**文件：**
- 修改：`Core.h`
- 修改：`Core.cpp`
- 修改：`FanControlProDlg.h`
- 修改：`FanControlProDlg.cpp`

**接口：**
- 消耗：`StartupState`、`CanWriteFans`。
- 产出：`CCore::GetInitError()`、`CCore::SetUserTakeoverAuthorized(BOOL)`、`WM_CORE_INIT_RESULT`。

- [ ] **步骤 1：编写核心状态测试**

扩展 `diagnostics/startup_state_test.cpp`，验证失败和未授权不能写入：

```cpp
assert(!CanWriteFans(StartupState::CoreFailed, true));
assert(!CanWriteFans(StartupState::CoreStarting, true));
assert(!CanWriteFans(StartupState::CoreReady, false));
```

运行任务 1 的编译命令。预期：现有实现下断言通过，作为硬件写入闸门的回归测试。

- [ ] **步骤 2：移除后台模态错误框并报告错误结果**

在 `CCore::Init` 和 `CConfig::SaveConfig` 的后台路径中，以包含错误码的 ASCII 日志及线程安全错误字符串替换 `AfxMessageBox`。核心线程在 `LoadConfig` 开始、结束、DLL 加载、导出解析、`InitIo` 返回和退出前各写一条日志，并在初始化成功或失败后：

```cpp
PostMessage(m_hWnd, WM_CORE_INIT_RESULT, succeeded ? 1 : 0, 0);
```

`OnCoreInitResult` 仅在 UI 线程中更新状态文字；失败时保持接管控件禁用并展示可读错误码。

- [ ] **步骤 3：禁止自动接管和启动写入**

新增 `std::atomic_bool m_userTakeoverAuthorized{false};` 和 `std::atomic_bool m_fansTouched{false};`。`Run` 加载持久化配置后必须将 `m_config.TakeOver` 置为 `FALSE`，直到用户点击接管复选框；`Init` 删除启动期间的 `SetFanDutyAuto` 循环。`SetFanDuty` 只能在下列条件成立时调用：

```cpp
CanWriteFans(m_startupState.load(), m_userTakeoverAuthorized.load())
```

明确用户勾选接管时调用 `SetUserTakeoverAuthorized(TRUE)`；首次实际写入后设置 `m_fansTouched=TRUE`。取消勾选或正常退出时，若 `m_fansTouched` 为真，先调用一次受控的 `SetFanDutyAuto` 恢复 BIOS 自动模式，再清除两个标志。除此之外，启动、失败和未授权路径绝不调用 `SetFanDutyAuto`。

- [ ] **步骤 4：验证核心失败与成功路径**

分别将 `ClevoEcInfo.dll` 暂时改名为 `ClevoEcInfo.dll.disabled` 再恢复原名，每次只启动一次程序。

失败预期：主窗口保留可读失败状态、无空白消息框、接管控件禁用、日志包含 DLL 加载错误码。

成功预期：日志包含 `Core.Init started`、`ClevoEcInfo.dll loaded`、`EC exports=1 InitIo=1` 和成功状态；在未勾选接管前日志中不得出现 `SetFanDuty` 或 `SetFanDutyAuto` 写入记录。

- [ ] **步骤 5：提交本任务**

```powershell
git add Core.h Core.cpp FanControlProDlg.h FanControlProDlg.cpp diagnostics/startup_state_test.cpp
git commit -m "fix: isolate startup failures and require fan takeover consent"
```

### 任务 4：将可选服务移出关键路径并完成发布验证

**文件：**
- 修改：`FanControlProDlg.cpp`
- 修改：`Release/RUN.md`
- 修改：`LOCAL_ADAPTATION.md`

**接口：**
- 消耗：任务 2 的 `OnDeferredStartup` 和任务 3 的 `WM_CORE_INIT_RESULT`。
- 产出：启动后可选任务日志与首次运行操作说明。

- [ ] **步骤 1：为可选操作编写失败场景检查**

在 `OnDeferredStartup` 的托盘添加和开机自启读取前后记录：

```cpp
WriteDiagnosticLog("Optional tray setup started");
WriteDiagnosticLog("Optional autorun status check started");
```

断开 Explorer 托盘可用性或令 `Shell_NotifyIcon` 返回失败时，窗口与 EC 初始化仍须完成；这是当前代码不保证的行为。

- [ ] **步骤 2：实现非关键可选任务**

`OnDeferredStartup` 先创建核心线程，再通过一次 `SetTimer` 或后续已投递消息执行 `SetTray` 和只读的 `SetAutorunReg(FALSE) || SetAutorunTask(FALSE)`。这些失败只写日志，不调用 `ExitProcess`、不显示消息框，也不改变 `StartupState::CoreReady`。

- [ ] **步骤 3：构建 Win32 Release**

运行：

```powershell
msbuild FanControlPro.sln /t:Rebuild /p:Configuration=Release /p:Platform=Win32 /m
```

预期：退出码为 `0`，`Release\FanControlPro.exe` 更新。

- [ ] **步骤 4：执行发布前人工验证**

1. 从 `Release` 目录启动 EXE，确认主窗口先出现且状态可读。
2. 读取 `FanControlPro.debug.log`，确认启动边界按顺序写入。
3. 不勾选接管，确认 BIOS 控制保持启用。
4. 仅在温度读数合理时勾选接管，观察两分钟后取消勾选并退出。
5. 归档 EXE、两个 DLL 与中文 `RUN.md` 到新的发行 ZIP；不包含驱动安装器。

- [ ] **步骤 5：提交本任务**

```powershell
git add FanControlProDlg.cpp Release/RUN.md LOCAL_ADAPTATION.md
git commit -m "docs: document safe Win32 startup validation"
```
