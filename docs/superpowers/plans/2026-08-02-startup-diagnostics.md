# 启动自检与本地诊断报告实施计划

## 全局约束

- 仅支持 `Debug|Win32` 与 `Release|Win32`。
- 自检只能调用既有读取接口；不得调用 `FnSetFanDuty`、`SetFANDutyAuto` 或其他 EC 写入接口。
- 自检失败时保留窗口和可用监控；任何风扇接管、手动占空比及强冷控制必须锁定。
- 诊断报告只在用户点击导出后创建为 UTF-8 文本，不上传网络，不写入用户名或完整本地路径。
- 新增纯 C++ 行为由 `diagnostics` 下的独立 C++17 用例覆盖；不提交生成的 `.exe`、`.obj`、`Debug/`、`Release/` 或 `.superpowers/`。

## Task 1: 只读启动自检策略

**文件：** 新建 `StartupSelfCheckPolicy.h`、`diagnostics/startup_self_check_policy_test.cpp`；修改 `StartupState.h`、`FanControlPro.vcxproj`。

**接口：** 提供不可变 `StartupCheckResult`，其中包含阻断故障、非阻断警告、连续三次温度采样计数、是否允许接管和中文状态说明。`StartupSelfCheckPolicy` 接受每轮 CPU/GPU 温度、RPM 与 GPU 可用性，三次有效双温度后才允许接管。RPM/GPU 不可用仅记录警告。温度不合理或核心初始化失败时返回仅监控状态。

**验证：** 先在测试中覆盖三次有效采样、一次无效温度锁定、RPM/GPU 警告不阻断、初始化失败阻断；观察测试因缺少头文件或接口而失败，再实现最小策略使其通过。

## Task 2: 诊断报告生成与脱敏

**文件：** 新建 `DiagnosticReport.h`、`diagnostics/diagnostic_report_test.cpp`；修改 `FanControlPro.vcxproj`。

**接口：** 接受不可变启动自检结果、程序/系统信息、传感器快照、接管状态与原始日志；输出 UTF-8 文本。保留最近 200 行日志，移除 Windows 绝对路径和用户名目录信息。报告没有文件系统或网络副作用，文件选择和写入由 UI 层负责。

**验证：** 测试报告包含必需字段、日志截断为 200 行、`C:\\Users\\name` 等本地路径被脱敏，并且 UTF-8 文本保留中文说明；先观察缺少头文件或接口的失败，再实现最小代码。

## Task 3: 核心与界面接入、文档和发布验证

**文件：** 修改 `Core.h`、`Core.cpp`、`FanControlProDlg.h`、`FanControlProDlg.cpp`、`FanControlPro.rc`、`resource.h`、`README.md`。

**行为：** 核心初始化后使用既有 EC 读取路径连续执行三轮自检，不新增任何写入调用。将只读自检快照暴露给 UI；失败时明确显示中文原因、保留监控并锁定控制。新增“导出诊断报告”按钮，通过保存对话框写入用户指定位置。报告收集当前 Windows 版本、管理员状态、DLL/驱动初始化状态、CPU/GPU 温度和 RPM、接管状态、错误码及脱敏日志。统一单实例提示为中文。README 记录诊断使用方式，并在最终 Release 构建后更新 EXE SHA-256。

**验证：** 运行全部 diagnostics；构建 Debug/Release Win32；执行缺少 DLL、无效温度和正常读取的人工测试路径，确认失败路径没有 EC 写入、正常路径仍要求用户显式勾选接管。
