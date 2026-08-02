# FanControl Pro Control Verification UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 显示 EC 风扇接管的实际写入与回读状态，并修复温度告警残留和高级设置默认展开问题。

**Architecture:** `ControlVerificationState` 提供独立、可测试的状态判定；`CCore` 用原子字段发布写入和回读数据；对话框读取快照，渲染控制状态和独立的告警文本。EC 写入、回读及现有 15% 阈值不改变。

**Tech Stack:** C++17、MFC、Win32、Visual Studio v143、Win32、x86 `ClevoEcInfo.dll`。

## Global Constraints

- 启动时 BIOS 控制保持启用；仅 `CoreReady && userAuthorized` 可写风扇。
- 不新增会改变风扇曲线或制造转速突变的“验证”命令。
- EC DLL 保持 ANSI 接口；不修改 EC 地址、导出名称或风扇算法。
- UI 线程只能读取原子控制快照，不能读取非原子 `m_bTakeOverStatus`。
- 默认阈值仍为 90 C，回差 5 C，通知冷却 10 分钟。

---

### Task 1: 可测试的控制状态判定与核心快照

**Files:**
- Create: `ControlVerificationState.h`
- Create: `diagnostics/control_verification_state_test.cpp`
- Modify: `Core.h`
- Modify: `Core.cpp`
- Modify: `FanControlPro.vcxproj`

**Interfaces:**
- Produces: `enum class ControlVerification { BiosControl, RequestingTakeover, Active, Fault };`
- Produces: `ControlVerification EvaluateControlVerification(StartupState, bool authorized, bool wrote, bool readbackMatches);`
- Produces: `CCore::GetControlVerification()` and atomic target/readback duty fields.

- [ ] **Step 1: 写失败测试**

```cpp
assert(EvaluateControlVerification(StartupState::CoreReady, false, false, false) == ControlVerification::BiosControl);
assert(EvaluateControlVerification(StartupState::CoreReady, true, false, false) == ControlVerification::RequestingTakeover);
assert(EvaluateControlVerification(StartupState::CoreReady, true, true, true) == ControlVerification::Active);
assert(EvaluateControlVerification(StartupState::CoreReady, true, true, false) == ControlVerification::Fault);
assert(EvaluateControlVerification(StartupState::CoreFailed, true, true, true) == ControlVerification::Fault);
```

- [ ] **Step 2: 编译测试并确认因缺少头文件失败**

Run: `cl /nologo /std:c++17 /EHsc diagnostics\control_verification_state_test.cpp /Fe:diagnostics\control_verification_state_test.exe`

Expected: 找不到 `ControlVerificationState.h`。

- [ ] **Step 3: 最小实现和核心集成**

实现无 MFC 依赖的判定函数。在 `SetFanDuty()` 成功调用 DLL 后记录目标占空比和 `wrote=true`；在 `Update()` 读取 EC 数据后记录回读占空比，并以 `abs(target-readback) <= EC_TAKEOVER_THRESHOLD` 更新 `readbackMatches`。提供只读原子访问器。

- [ ] **Step 4: 运行控制状态、启动状态与温度告警独立测试，并构建 Debug/Release Win32**

Expected: 三个测试和两个构建均退出 0。

- [ ] **Step 5: 提交**

```powershell
git add ControlVerificationState.h diagnostics/control_verification_state_test.cpp Core.h Core.cpp FanControlPro.vcxproj
git commit -m "feat: expose verified fan control state"
```

### Task 2: 默认界面控制状态与告警修复

**Files:**
- Modify: `resource.h`
- Modify: `FanControlPro.rc`
- Modify: `FanControlProDlg.h`
- Modify: `FanControlProDlg.cpp`

**Interfaces:**
- Consumes: `CCore::GetControlVerification()`、原子目标/回读占空比和 `IsTemperatureWarning()`。
- Produces: 独立的 `IDC_STATIC_CONTROL_STATUS` 与 `IDC_STATIC_WARNING_STATUS`。

- [ ] **Step 1: 先声明并使用未定义的控制状态更新函数，确认链接失败**

添加两个资源 ID 和 `void UpdateControlStatus();`，在 `UpdateGui()` 调用该函数但暂不实现。

- [ ] **Step 2: 构建 Debug，确认缺少实现导致失败**

Run: `MSBuild FanControlPro.sln /t:Build /p:Configuration=Debug /p:Platform=Win32 /m`

Expected: 链接错误指向 `UpdateControlStatus`。

- [ ] **Step 3: 实现 UI**

资源中加入独立控制状态、告警状态文本。默认调用 `SetAdvancedMode(FALSE)`，并在资源布局中为高级设置留出仅展开时使用的区域。`UpdateControlStatus()` 依据枚举显示：`BIOS 控制中`、`正在请求接管`、`接管已生效：CPU 目标 %d%% / EC %d%%，GPU 目标 %d%% / EC %d%%`、`接管状态异常`。`UpdateWarningStatus()` 必须在告警解除时设置 `温度正常`，不能覆盖启动状态。

- [ ] **Step 4: 构建 Debug/Release，并在真实目录启动 Release**

Expected: 两个构建成功；未勾选接管显示 BIOS 控制，程序正常读取 EC；部署前备份现有 EXE。

- [ ] **Step 5: 提交**

```powershell
git add resource.h FanControlPro.rc FanControlProDlg.h FanControlProDlg.cpp
git commit -m "feat: show verified fan takeover state"
```
