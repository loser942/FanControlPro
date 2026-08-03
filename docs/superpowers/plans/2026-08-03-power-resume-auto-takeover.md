# 睡眠恢复与自动接管实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 让程序在开机和睡眠恢复后经过只读自检再安全自动接管，并使接管故障可通过重新自检恢复。

**Architecture:** 使用独立 `PowerResumePolicy` 描述恢复会话，`CCore` 在核心线程消费 UI 发送的电源事件并重置传感器、接管和启动自检会话。自动接管偏好作为配置字段持久化，但其生效必须经过现有启动自检和 `CanWriteFans()` 门控。

**Tech Stack:** C++17、MFC、Win32 电源广播、现有 Win32 独立策略测试、MSBuild Win32。

## Global Constraints

- 目标硬件保持为已验证的 Clevo EC 笔记本，不扩展 EC 映射或设备支持范围。
- `WM_POWERBROADCAST` 的 UI 处理函数不调用 EC DLL 或风扇写入接口。
- 启动、恢复和手动重试都必须先完成三次只读温度自检，失败时保持 BIOS 控制。
- `AutoTakeoverAfterCheck` 默认关闭，仅由用户主动启用并持久化。

---

### Task 1: 恢复会话策略与测试

**Files:**
- Create: `PowerResumePolicy.h`
- Create: `diagnostics/power_resume_policy_test.cpp`
- Modify: `FanControlPro.vcxproj`

**Interfaces:**
- Produces: `PowerResumePolicy::BeginRecovery(bool)`, `RecordSelfCheck(bool)`, `CanWriteFans() const`, `ShouldRequestAutoTakeover() const`, `ResetForManualRetry()`。

- [ ] **Step 1: 写入失败测试**

```cpp
PowerResumePolicy policy;
policy.BeginRecovery(true);
assert(!policy.CanWriteFans());
policy.RecordSelfCheck(true);
policy.RecordSelfCheck(true);
assert(!policy.ShouldRequestAutoTakeover());
policy.RecordSelfCheck(true);
assert(policy.ShouldRequestAutoTakeover());
```

- [ ] **Step 2: 运行并确认失败**

Run: `cl /std:c++17 /EHsc diagnostics\\power_resume_policy_test.cpp /Fe:diagnostics\\power_resume_policy_test.exe`

Expected: FAIL，类型尚未定义。

- [ ] **Step 3: 最小实现并复验**

恢复开始时禁止写入，连续三次通过后才产生一次自动接管请求；运行上述测试并确认 PASS。

### Task 2: 配置迁移与自动接管偏好

**Files:**
- Modify: `ConfigPersistence.h`
- Modify: `Core.h`
- Modify: `Core.cpp`
- Modify: `diagnostics/config_persistence_test.cpp`

**Interfaces:**
- Produces: `CConfig::AutoTakeoverAfterCheck`，旧 V4 配置读取时默认 `FALSE`。

- [ ] **Step 1: 写入失败测试并确认失败**

```cpp
CConfig config;
config.LoadDefault();
assert(config.AutoTakeoverAfterCheck == FALSE);
config.AutoTakeoverAfterCheck = TRUE;
assert(RoundTrip(config).AutoTakeoverAfterCheck == TRUE);
```

- [ ] **Step 2: 最小实现并复验**

新增磁盘版本而不是修改 V4 结构；旧 V4 迁移时新字段为 `FALSE`，写入使用新版本。运行配置测试并确认 PASS。

### Task 3: 核心恢复状态机

**Files:**
- Modify: `Core.h`
- Modify: `Core.cpp`
- Modify: `StartupSelfCheckPolicy.h`
- Modify: `diagnostics/startup_self_check_policy_test.cpp`

**Interfaces:**
- Produces: `CCore::NotifyPowerSuspend()`, `NotifyPowerResume()`, `RequestManualTakeoverRetry()`。

- [ ] **Step 1: 写入失败测试并确认失败**

```cpp
StartupSelfCheckPolicy check;
check.Reset();
assert(!check.Evaluate(true, 55, 50, 0, 0, false).takeoverAllowed);
assert(!check.Evaluate(true, 55, 50, 0, 0, false).takeoverAllowed);
assert(check.Evaluate(true, 55, 50, 0, 0, false).takeoverAllowed);
```

- [ ] **Step 2: 最小实现并复验**

核心线程消费挂起/恢复请求；恢复或手动重试时重置传感器、接管验证、目标占空比和自检，进入 `SelfChecking`。三次采样成功后按自动偏好或手动授权请求接管。

### Task 4: MFC 电源消息、UI、日志与发布构建

**Files:**
- Modify: `FanControlProDlg.h`
- Modify: `FanControlProDlg.cpp`
- Modify: `FanControlPro.rc`
- Modify: `resource.h`
- Modify: `DiagnosticReport.h`
- Modify: `README.md`
- Modify: `diagnostics/diagnostic_report_test.cpp`

- [ ] **Step 1: 写入失败测试并确认失败**

在诊断报告测试中加入恢复自检日志样本并断言其输出。

- [ ] **Step 2: 最小实现**

添加持久化复选框“自检通过后自动接管”及 `ON_WM_POWERBROADCAST()`；UI 只通知核心并显示恢复状态，诊断记录挂起、恢复、自检与自动接管请求。

- [ ] **Step 3: 全量验证与提交**

运行全部独立诊断测试，重建 `Debug|Win32` 与 `Release|Win32`，运行 `git diff --check`，更新 README 发布哈希，再提交本轮改动。

### Task 5: 长时稳定性验证

**Files:**
- Modify: `diagnostics/power_resume_policy_test.cpp`
- Modify: `README.md`

- [ ] **Step 1: 自动化循环耐久测试**

模拟 10,000 次恢复会话，交替运行自动接管、手动模式和失败自检。每轮断言恢复开始时禁止写入，成功后才允许写入，自动请求只能消费一次。

- [ ] **Step 2: 实机浸泡测试说明**

在 README 中提供用户执行的 2 小时日常运行与至少 3 次睡眠恢复检查清单。不得在自动化验证中强制系统休眠或未经用户操作实际写入 EC。
