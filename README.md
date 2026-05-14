# FanControl Pro

> 🔧 本项目基于 [xl-Synapse/MyFanControl](https://github.com/xl-Synapse/MyFanControl) 修改而来

**蓝天 (Clevo) 模具笔记本智能风扇控制工具**

[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)]()
[![Language](https://img.shields.io/badge/language-C%2B%2B%20MFC-orange)]()
[![License](https://img.shields.io/badge/license-MIT-green)]()

---

## ✨ 特性

| 类别 | 功能 |
|------|------|
| 🎮 控制模式 | 自动（阶梯/线性）、手动、强冷 |
| 🌡️ 温度曲线 | CPU/GPU 独立 10 档调速，针对 i5-13500H + RTX 4060 优化 |
| 🔊 平滑过渡 | 升速 +4%/次、降速 -2%/次，消除风扇忽高忽低 |
| 🛡️ EC 防抢夺 | 独立每秒刷新 + 接管检测自动夺回 |
| 💥 崩溃安全 | 异常退出后启动时自动归还 BIOS 控制权 |
| 🔍 写后验证 | 读取 EC 实际 duty 对比写入值，偏差 >15% 自动夺回 |
| 🔐 线程安全 | CRITICAL_SECTION 锁护 m_config 跨线程访问，热路径快照隔离 |
| 🔒 单实例 | CreateMutex 互斥，防止多进程争抢 EC |
| 🎨 Win11 UI | 温度进度条、实时 RPM 显示、托盘告警 |
| 🚀 开机自启 | 注册表 + 任务计划双保险 |
| ⚡ 预设切换 | 静音/性能/平衡一键切换 |

## 🖥️ 适用机型

- **硬件**：蓝天 (Clevo) 模具笔记本
- **测试配置**：i5-13500H + RTX 4060 + Windows 11

## 🔧 构建与使用

1. **Visual Studio 2022**（含 C++ MFC 支持）打开 `FanControlPro.sln`
2. 编译生成 `FanControlPro.exe`
3. 从 [Releases](../../releases) 下载 `MyFanControl-v1.0.zip`（运行时依赖）
4. 解压后放入编译输出目录（与 `FanControlPro.exe` 同级）
5. 运行 `NTPortDrvSetup.exe` 安装驱动

> ⚠️ **重要提示**：Releases 中的 Zip 包仅包含运行时 DLL 依赖，**不含可执行程序**，必须先编译本项目。

## 📋 版本历史

| 版本 | 改进内容 |
|------|---------|
| **v21** | 🛡️ openclaw 审查 2 项：GPU DLL 全部 19 个函数指针纳入安全检查 / 强冷退出还原保存的 ControlMode（非硬编码 0） |
| **v20** | 🛡️ 本轮审查 5 项：CGPUInfo 构造器全部指针/成员归零 / 温度异常 3 次后保留旧值 / CreateThread 失败弹错退出 / OnTimer>300 OnOK 后立即 return / ExecuteCmd WaitForSingleObject 10s 超时终止 |
| **v19** | 🔧 Claude 审查 8 项：OnOK ExitProcess+句柄关闭 / m_pfnSetFanDuty 判空 / Update 首帧跳过重试 / OnTimer 防重入 / ExecuteCmd 动态缓冲+ReadFile 修复 / Autorun 返回值检查 |
| **v18** | 🔧 互斥体句柄泄漏修复（CreateMutex→保存→OnOK释放）+ GetExePath 改用 strrchr 增强健壮性 |
| **v17** | 🛡️ GPT 3项高优：GPU DLL 全函数 NULL 检查（Update/LockFrequency 路径）/ 失败路径成员归零 / 线程卡死 ExitProcess 原子退出防竞态 |
| **v16** | 🛡️ m_pfnGetTempFanDuty NULL 检查：Init() 拒绝初始化 + Update() 调用前防御 return，消除最后一个 DLL 函数指针空调用风险 |
| **v15** | 🔥 v14 编译错误热修复：LoadConfig 缺 `}` / SaveConfig 多 `)` / OnTimer 死代码清理 |
| **v14** | 🐛 OP 审查 7 项修复：OnInitDialog 签名 void→BOOL、TRACE1 参数溢出→TRACE、m_bForcedCooling 加锁快照、计数器误判修复、OnOK 防 UAF、fwrite/fread 返回值检查、GetBuffer 补 ReleaseBuffer |
| **v13** | 🛡️ 稳定性加固：CGPUInfo DLL 关键函数 NULL 检查防崩溃（`InitGPU_API`/`Check_GPU_VRAM_Clock`/`CloseGPU_API`）、线程超时后不 CloseHandle 不 ResetFan 防退出竞态 |
| **v12** | 🐛 5 项关键修复：配置序列化 Bug（offsetof 含 ConfigPath 导致从未保存配置，v2 格式修复）、窗口恢复（m_bForceHideWindow=FALSE）、强冷退出重置 ControlMode、XML 声明改 UTF-8、注册表 REG_SZ 含 null+字节数、CPU 使用率显示、菜单句柄泄漏 |
| **v11** | 🔒 5 项安全加固：退出事件信号替代 Sleep+TerminateThread（防死锁）、单实例互斥、m_bForcedCooling 统一加锁、温度异常最大 2 次重试、ExecuteCmd memset 清零 |
| **v10** | 🔐 m_config 线程安全：CRITICAL_SECTION 锁护所有跨线程访问，`Work()` 热路径快照隔离，UI 与工作线程零竞争 |
| **v9** | 🔍 写后验证 + EC 接管检测：读取 EC 实际 duty 对比写入值，偏差 >15% 自动夺回 |
| **v8** | 🧼 代码净化：`constexpr` 常量替代 `#define`，RAII `DllHandle` 类消除裸 `FreeLibrary`，魔法数字全部具名常量化 |
| **v7** | 🐛 修复 4 个 Bug：Silent 预设累积退化、GetExePath 边界检查、告警 ID 修正、配置文件版本魔数；3 项风险：午夜回绕保护、RPM 间隙值标记 |
| **v6** | 🛡️ Clevo EC 防抢夺：独立每秒刷新机制，对抗 EC 看门狗 |
| **v5** | 💥 崩溃安全加固：三层 BIOS 归还（正常退出 / TerminateThread / 启动恢复） |
| **v4** | 🔊 平滑风扇过渡：升速 +4%/次、降速 -2%/次 |
| **v3** | 🔒 并发安全：6 处 static 局部变量改为成员变量 |
| **v2** | 🔧 14 项崩溃/逻辑修复 |
| **v1** | 🎉 初始重构：三种控制模式 + Win11 UI |

## 📄 协议

MIT License