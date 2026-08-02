# FanControl Pro 安全性加固实施计划

> **面向执行代理：** 必须使用 `superpowers:subagent-driven-development`（推荐）或 `superpowers:executing-plans` 逐任务执行。本计划使用 `- [ ]` 复选框跟踪步骤。

**目标：** 在硬件状态、传感器或配置不可靠时停止 EC 写入并恢复 BIOS 控制，同时修复自启和 Unicode 路径问题。

**架构：** 把可判断的安全规则抽为无 MFC 依赖的状态机：`TakeoverVerificationPolicy` 负责连续回读、超时和抢回预算，`SensorHealthPolicy` 负责采样故障。`CCore` 仅将真实写入、读回和退出事件映射到这些策略；配置保存使用明确的 v4 磁盘结构及原子替换。MFC 对话框只渲染状态和调用清晰的自启操作。

**技术栈：** C++17、Win32 API、静态 MFC、Visual Studio 2022 v143、MSBuild、Win32、x86 `ClevoEcInfo.dll`。

## 全局约束

- 仅构建 `Debug|Win32` 和 `Release|Win32`，不得新增 x64 目标。
- 未满足 `CoreReady && userAuthorized` 时，普通控制、强冷和热紧急都不得调用 `SetFanDuty`。
- 不改变 EC DLL 的导出名称、函数签名或寄存器映射；`GetProcAddress` 仍使用 ASCII 导出名。
- 保持对 v3 配置读取兼容；后续保存必须写 v4 明确磁盘结构。
- 所有新增策略和持久化规则必须由 `diagnostics` 中可单独编译的 C++17 用例覆盖。
- 不提交 `diagnostics/*.exe`、`.superpowers/`、`Debug/` 或 `Release/` 构建输出。

## 文件职责

- `TakeoverVerificationPolicy.h`：连续回读、超时、重新夺回限额及故障回退的纯 C++ 状态机。
- `SensorHealthPolicy.h`：每轮双传感器采样有效性与连续失败计数的纯 C++ 状态机。
- `ConfigPersistence.h`：明确 v4 磁盘头、原子写入、备份恢复和 v3 原始字节读取的 Win32 文件工具。
- `diagnostics/*_test.cpp`：每个策略与持久化行为的独立回归测试。
- `Core.h` / `Core.cpp`：把策略状态、宽字符 DLL 路径、配置快照和核心线程内复位接入真实控制循环。
- `FanControlProDlg.h` / `FanControlProDlg.cpp`：分离自启查询/创建/删除，使用宽字符进程执行，并确保未授权强冷不会伪装为开启。
- `FanControlPro.vcxproj`：列出新增头文件。

---

### 任务 1：接管确认会话与抢回预算

**文件：**

- 新建：`TakeoverVerificationPolicy.h`
- 修改：`ControlVerificationState.h`
- 修改：`diagnostics/control_verification_state_test.cpp`
- 修改：`Core.h`
- 修改：`Core.cpp`
- 修改：`FanControlPro.vcxproj`

**接口：**

- 产出：`class TakeoverVerificationPolicy`，含 `RecordWrite(int cpuTarget, int gpuTarget, unsigned long long nowMs)`、`RecordReadback(int cpuDuty, int gpuDuty, unsigned long long nowMs)`、`bool CanReclaim(unsigned long long nowMs)`、`bool IsFaulted() const` 与 `ControlVerification State() const`。
- 消费：`EC_TAKEOVER_THRESHOLD == 15`、核心线程的已发出写入和 EC 回读。
- 行为：同一目标的两次连续回读匹配后才为 `Active`；目标改变、过期回读或偏差都会重新请求；五秒窗口内第三次抢回后为 `Fault`。

- [ ] **步骤 1：编写失败测试**

在 `diagnostics/control_verification_state_test.cpp` 增加以下契约，并包含尚不存在的 `TakeoverVerificationPolicy.h`：

```cpp
TakeoverVerificationPolicy policy(15, 3000, 2, 5000, 3);
policy.RecordWrite(60, 65, 100);
assert(policy.State() == ControlVerification::RequestingTakeover);
policy.RecordReadback(62, 63, 200);
assert(policy.State() == ControlVerification::RequestingTakeover);
policy.RecordReadback(61, 64, 300);
assert(policy.State() == ControlVerification::Active);

policy.RecordWrite(80, 80, 400);
assert(policy.State() == ControlVerification::RequestingTakeover);
policy.RecordReadback(20, 20, 500);
assert(policy.CanReclaim(500));
assert(policy.CanReclaim(600));
assert(!policy.CanReclaim(700));
assert(policy.IsFaulted());
```

- [ ] **步骤 2：运行测试，确认其因缺少策略接口失败**

运行：

```powershell
cl /nologo /EHsc /std:c++17 /Fe:diagnostics\control_verification_state_test.exe diagnostics\control_verification_state_test.cpp
```

预期：编译失败，提示找不到 `TakeoverVerificationPolicy.h` 或类型未定义；不能因断言失败以外的原因失败。

- [ ] **步骤 3：实现最小策略并接入核心线程**

在 `TakeoverVerificationPolicy.h` 定义构造参数、目标快照、连续匹配计数、最后写入时间、窗口内抢回次数和 `Fault` 状态。`RecordReadback` 仅在写入后的 3000 ms 内比较两路风扇；任一偏差清零匹配次数。`CanReclaim` 在五秒窗口内只允许两次为真，第三次设置 `Fault`。

在 `CCore::SetFanDuty()` 调用 DLL 后仅调用 `m_takeoverVerification.RecordWrite(...)`，不再设置“写入成功”布尔值。在 `Update()` 获取两路读回值后调用 `RecordReadback(...)`。`VerifyAndReclaim()` 必须先检查 `CanReclaim(GetTickCount64())`；预算耗尽时调用 `ResetFan()` 并停止后续写入。`GetControlVerification()` 返回策略的状态。

- [ ] **步骤 4：重新编译并运行接管、启动和告警测试**

运行：

```powershell
cl /nologo /EHsc /std:c++17 /Fe:diagnostics\control_verification_state_test.exe diagnostics\control_verification_state_test.cpp
& .\diagnostics\control_verification_state_test.exe
& .\diagnostics\startup_state_test.exe
& .\diagnostics\temperature_alert_policy_test.exe
```

预期：三个进程均返回 `0`；接管测试证明单次匹配不是 Active，第三次抢回使策略 Fault。

- [ ] **步骤 5：提交本任务**

```powershell
git add TakeoverVerificationPolicy.h ControlVerificationState.h Core.h Core.cpp FanControlPro.vcxproj diagnostics/control_verification_state_test.cpp
git commit -m "fix: require sustained fan takeover verification"
```

### 任务 2：传感器故障与线程内 BIOS 回退

**文件：**

- 新建：`SensorHealthPolicy.h`
- 新建：`diagnostics/sensor_health_policy_test.cpp`
- 修改：`Core.h`
- 修改：`Core.cpp`
- 修改：`FanControlPro.vcxproj`

**接口：**

- 产出：`class SensorHealthPolicy`，含 `bool RecordCycle(bool cpuValid, bool gpuValid)`、`bool IsFaulted() const`、`void Reset()`；连续三轮任一路无效时返回 `false` 并进入故障。
- 消费：`CCore::Update()` 的两路采样结果和 `m_hExitEvent`。
- 行为：异常重试通过 `WaitForSingleObject(m_hExitEvent, 1000)` 中断；故障后 `Work()` 不再调用 `VerifyAndReclaim`、`Control` 或 `SetFanDuty`，在核心线程调用一次 `ResetFan()`。

- [ ] **步骤 1：编写失败测试**

创建 `diagnostics/sensor_health_policy_test.cpp`：

```cpp
#include "../SensorHealthPolicy.h"
#include <cassert>

int main()
{
    SensorHealthPolicy policy(3);
    assert(policy.RecordCycle(true, true));
    assert(policy.RecordCycle(false, true));
    assert(policy.RecordCycle(false, true));
    assert(!policy.RecordCycle(false, true));
    assert(policy.IsFaulted());
    policy.Reset();
    assert(!policy.IsFaulted());
    assert(policy.RecordCycle(true, true));
    return 0;
}
```

- [ ] **步骤 2：运行测试，确认其因缺少头文件失败**

运行：

```powershell
cl /nologo /EHsc /std:c++17 /Fe:diagnostics\sensor_health_policy_test.exe diagnostics\sensor_health_policy_test.cpp
```

预期：编译失败，提示缺少 `SensorHealthPolicy.h`。

- [ ] **步骤 3：实现策略并收紧核心退出路径**

实现 `SensorHealthPolicy`，任何完整有效轮次清零连续失败计数。重写 `Update()`，先完成一轮两路采样，再提交温度和 RPM；异常等待改为 `WaitForSingleObject`，收到退出事件时立即返回。`Work()` 发现策略故障后将控制验证置 Fault，调用 `ResetFan()` 并直接返回。

将 `Uninit()` 改为只关闭 DLL、清理初始化状态；保留 `Run()` 退出、传感器故障和接管故障三条核心线程内的 `ResetFan()` 调用。

- [ ] **步骤 4：运行新增和现有策略测试**

运行：

```powershell
cl /nologo /EHsc /std:c++17 /Fe:diagnostics\sensor_health_policy_test.exe diagnostics\sensor_health_policy_test.cpp
& .\diagnostics\sensor_health_policy_test.exe
& .\diagnostics\control_verification_state_test.exe
& .\diagnostics\startup_state_test.exe
```

预期：所有进程返回 `0`；连续三轮故障被锁定，重置后可重新采样。

- [ ] **步骤 5：提交本任务**

```powershell
git add SensorHealthPolicy.h diagnostics/sensor_health_policy_test.cpp Core.h Core.cpp FanControlPro.vcxproj
git commit -m "fix: stop fan control on sensor faults"
```

### 任务 3：v4 配置与原子快照保存

**文件：**

- 新建：`ConfigPersistence.h`
- 新建：`diagnostics/config_persistence_test.cpp`
- 修改：`Core.h`
- 修改：`Core.cpp`
- 修改：`FanControlProDlg.cpp`
- 修改：`FanControlPro.vcxproj`

**接口：**

- 产出：`struct ConfigV4Disk`，显式包含 `DutyList`、`TransitionTemp`、`UpdateInterval`、`Linear`、`TakeOver`、`ForceTemp`、`MaxDutyLimit`、`LockGPUFrequency`、`GPUFrequency`、`ControlMode`、`ManualDuty`、`WarningTemp`、`DesktopNotifications`、`NotificationCooldownMinutes`。
- 产出：`bool WriteConfigAtomically(PCWSTR path, const void* bytes, size_t byteCount)` 与 `bool ReadConfigWithBackup(PCWSTR path, void* bytes, size_t byteCount)`。
- 产出：`CCore::SaveConfigSnapshot()`；它在配置锁内复制 `CConfig`，锁外保存快照。

- [ ] **步骤 1：编写失败测试**

创建 `diagnostics/config_persistence_test.cpp`，在临时目录中验证 v4 主文件、备份和未完成临时文件：

```cpp
ConfigV4Disk expected{};
expected.UpdateInterval = 2;
assert(WriteConfigAtomically(configPath.c_str(), &expected, sizeof(expected)));
ConfigV4Disk loaded{};
assert(ReadConfigWithBackup(configPath.c_str(), &loaded, sizeof(loaded)));
assert(loaded.UpdateInterval == 2);

CopyFileW(configPath.c_str(), backupPath.c_str(), FALSE);
std::ofstream(configPath, std::ios::binary | std::ios::trunc).write("bad", 3);
assert(ReadConfigWithBackup(configPath.c_str(), &loaded, sizeof(loaded)));
assert(loaded.UpdateInterval == 2);
```

- [ ] **步骤 2：运行测试，确认其因缺少持久化接口失败**

运行：

```powershell
cl /nologo /EHsc /std:c++17 /Fe:diagnostics\config_persistence_test.exe diagnostics\config_persistence_test.cpp
```

预期：编译失败，提示 `ConfigPersistence.h`、`ConfigV4Disk` 或读写函数缺失。

- [ ] **步骤 3：实现持久化和核心快照接口**

实现带 magic、version 和 payload 大小的 v4 文件头。写入使用 `<path>.tmp`、`FlushFileBuffers`、`ReplaceFileW` 或 `MoveFileExW(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`；在替换前保留 `<path>.bak`。读取先验证主文件的头和完整 payload，失败时验证 `.bak`。保留 v3 读取分支，将旧内存布局字段复制至 `ConfigV4Disk` 后归一化。

在 `CCore` 中添加 `SaveConfigSnapshot()`，所有 UI 与核心调用先在锁内更新字段，再通过该接口保存。删除所有解锁后的 `m_config.SaveConfig()` 调用；`CConfig` 的读写只由核心配置生命周期使用。

- [ ] **步骤 4：重新编译并运行配置回归测试**

运行：

```powershell
cl /nologo /EHsc /std:c++17 /Fe:diagnostics\config_persistence_test.exe diagnostics\config_persistence_test.cpp
& .\diagnostics\config_persistence_test.exe
```

预期：进程返回 `0`；截断主文件后读取备份，未完成 `.tmp` 不被当作有效配置。

- [ ] **步骤 5：提交本任务**

```powershell
git add ConfigPersistence.h diagnostics/config_persistence_test.cpp Core.h Core.cpp FanControlProDlg.cpp FanControlPro.vcxproj
git commit -m "fix: persist fan settings atomically"
```

### 任务 4：宽字符路径、强冷授权和自启操作

**文件：**

- 新建：`AutorunCommandPolicy.h`
- 新建：`diagnostics/autorun_command_policy_test.cpp`
- 修改：`StartupState.h`
- 修改：`diagnostics/startup_state_test.cpp`
- 修改：`Core.h`
- 修改：`Core.cpp`
- 修改：`FanControlProDlg.h`
- 修改：`FanControlProDlg.cpp`
- 修改：`FanControlPro.vcxproj`

**接口：**

- 产出：`bool CanEnableForcedCooling(StartupState state, bool userAuthorized)`，与 `CanWriteFans` 同一条件。
- 产出：`bool IsExactAutorunCommand(const std::wstring& stored, const std::wstring& exePath)`，只接受带引号、等于当前 EXE 的注册表命令。
- 产出：`QueryAutorunReg()`、`CreateAutorunReg()`、`DeleteAutorunReg()`、`QueryAutorunTask()`、`CreateAutorunTask()`、`DeleteAutorunTask()` 和 `RunHiddenCommand(PCWSTR commandLine, DWORD* exitCode)`。

- [ ] **步骤 1：编写失败测试**

扩展 `diagnostics/startup_state_test.cpp`：

```cpp
assert(!CanEnableForcedCooling(StartupState::CoreReady, false));
assert(CanEnableForcedCooling(StartupState::CoreReady, true));
```

创建 `diagnostics/autorun_command_policy_test.cpp`：

```cpp
#include "../AutorunCommandPolicy.h"
#include <cassert>

int main()
{
    const std::wstring path = L"C:\\测试目录\\FanControlPro.exe";
    assert(IsExactAutorunCommand(L"\"C:\\测试目录\\FanControlPro.exe\"", path));
    assert(!IsExactAutorunCommand(L"C:\\旧目录\\FanControlPro.exe", path));
    assert(!IsExactAutorunCommand(L"\"C:\\测试目录\\FanControlPro.exe\" --legacy", path));
    return 0;
}
```

- [ ] **步骤 2：运行测试，确认新接口尚不存在**

运行：

```powershell
cl /nologo /EHsc /std:c++17 /Fe:diagnostics\startup_state_test.exe diagnostics\startup_state_test.cpp
cl /nologo /EHsc /std:c++17 /Fe:diagnostics\autorun_command_policy_test.exe diagnostics\autorun_command_policy_test.cpp
```

预期：第一条因 `CanEnableForcedCooling` 未定义失败，第二条因 `AutorunCommandPolicy.h` 未定义失败。

- [ ] **步骤 3：实现宽字符硬件路径与明确的 UI 操作**

将全局 `GetExePath()` 改为返回 `CStringW`，使用 `GetModuleFileNameW`。`DllHandle::Load` 接收 `PCWSTR` 并调用 `LoadLibraryW`；EC DLL 与 GPU DLL 都传递宽字符完整路径。任务 XML 使用 `CreateFileW` / UTF-8 内容，任务命令使用 `CreateProcessW`，不建立输出管道；仅根据退出码判定成功。

把原来的二义性 `SetAutorunReg` / `SetAutorunTask` 替换为查询、创建、删除函数。取消勾选必须调用两个删除函数一次，再以两个查询函数刷新复选框。未授权点击强冷或托盘强冷时，恢复复选框为未选中并显示“请先启用风扇接管”。

- [ ] **步骤 4：编译并运行授权与自启策略测试**

运行：

```powershell
cl /nologo /EHsc /std:c++17 /Fe:diagnostics\startup_state_test.exe diagnostics\startup_state_test.cpp
cl /nologo /EHsc /std:c++17 /Fe:diagnostics\autorun_command_policy_test.exe diagnostics\autorun_command_policy_test.cpp
& .\diagnostics\startup_state_test.exe
& .\diagnostics\autorun_command_policy_test.exe
```

预期：两个进程返回 `0`；带参数或旧路径的注册表命令不会被认定为当前版本自启。

- [ ] **步骤 5：提交本任务**

```powershell
git add AutorunCommandPolicy.h StartupState.h diagnostics/startup_state_test.cpp diagnostics/autorun_command_policy_test.cpp Core.h Core.cpp FanControlProDlg.h FanControlProDlg.cpp FanControlPro.vcxproj
git commit -m "fix: harden autorun and Unicode paths"
```

### 任务 5：全量验证、中文说明与发布前检查

**文件：**

- 修改：`README.md`
- 修改：`docs/superpowers/specs/2026-08-02-safety-hardening-design.md`
- 修改：`docs/superpowers/plans/2026-08-02-safety-hardening.md`

**接口：**

- 消费：前四项任务的所有诊断 EXE 与 `FanControlPro.sln`。
- 产出：可复现的 Debug/Release Win32 构建证据、更新后的用户安全说明。

- [ ] **步骤 1：运行所有独立诊断测试**

运行：

```powershell
Get-ChildItem diagnostics\*_test.cpp | ForEach-Object {
  $name = $_.BaseName
  cl /nologo /EHsc /std:c++17 /Fe:("diagnostics\\{0}.exe" -f $name) $_.FullName
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  & (".\\diagnostics\\{0}.exe" -f $name)
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
```

预期：所有诊断程序均返回 `0`。

- [ ] **步骤 2：构建 Win32 Debug 与 Release**

运行：

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe' FanControlPro.sln /t:Rebuild /p:Configuration=Debug /p:Platform=Win32 /m
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe' FanControlPro.sln /t:Rebuild /p:Configuration=Release /p:Platform=Win32 /m
```

预期：两个命令退出码均为 `0`，并且 `Release\FanControlPro.exe` 更新。

- [ ] **步骤 3：更新 README 的安全行为说明**

在 README 的“使用说明”和“常见问题”中说明：接管状态需要连续 EC 回读确认；传感器、EC 验证或抢回预算故障时程序自动归还 BIOS；强冷必须先授权；配置崩溃恢复会尝试 `.bak`。

- [ ] **步骤 4：执行发布前静态检查**

运行：

```powershell
git diff --check origin/main...HEAD
rg -n "LoadLibraryA|GetModuleFileNameA|Sleep\(1000\)|rs\.Find\(\"成功\"\)|SetAutorunReg\(FALSE\)" Core.cpp FanControlProDlg.cpp
git status -sb
```

预期：无 diff 空白错误；旧 ANSI 模块路径、不可中断重试、中文命令输出判断及二义性自启调用均无命中；只有明确排除的本地诊断 EXE 和 `.superpowers/` 保持未跟踪。

- [ ] **步骤 5：提交验证与文档**

```powershell
git add README.md docs/superpowers/specs/2026-08-02-safety-hardening-design.md docs/superpowers/plans/2026-08-02-safety-hardening.md
git commit -m "docs: document fan control safety behavior"
```
