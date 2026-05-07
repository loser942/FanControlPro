# FanControl Pro

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
| 🛡️ EC 防抢夺 | 独立每秒刷新，对抗 Clevo EC 固件看门狗（3-5 秒回退） |
| 💥 崩溃安全 | 异常退出后启动时自动归还 BIOS 控制权，防止风扇卡死 |
| 🎨 Win11 UI | 深色主题、温度进度条、实时 RPM 显示 |
| 🔔 温度告警 | CPU/GPU 超温托盘气泡通知 |
| 🚀 开机自启 | 注册表 + 任务计划双保险 |
| ⚡ 预设切换 | 静音/性能/平衡一键切换 |

## 🖥️ 适用机型

- **硬件**：蓝天 (Clevo) 模具笔记本（EC 接口兼容机型）
- **测试配置**：i5-13500H + RTX 4060 + Windows 11 25H2
- **依赖 DLL**：
  - `ClevoEcInfo.dll` — 风扇 EC 控制接口
  - `NVGPU_DLL.dll` — GPU 频率读取接口

## 📋 版本历史

| 版本 | 改进 |
|------|------|
| v1 | 初始重构：三种控制模式 + Win11 UI |
| v2 | 14 项崩溃/逻辑修复（CConfig 序列化、线程安全等） |
| v3 | 6 项并发安全（static 局部变量 → 成员变量） |
| v4 | 平滑风扇过渡（+4/-2 渐变） |
| v5 | 崩溃安全退出（三层 BIOS 归还防护） |
| v6 | Clevo EC 防抢夺（双时钟架构） |
| **v7** | **4 Bug + 3 风险修复（预设累积、向前兼容等）** |

## 🔧 构建

### 环境要求
- Visual Studio 2022（含 C++ MFC 支持）
- Windows SDK 10.0+

### 编译步骤
1. 克隆仓库
2. 将 `ClevoEcInfo.dll` 和 `NVGPU_DLL.dll` 放入输出目录
3. 打开 `FanControlPro.sln` → 生成解决方案

## ⚠️ 安全声明

> 本程序直接操作 EC（嵌入式控制器），违反 EC 规范操作可能导致：
> - 风扇停转导致过热
> - EC 固件异常
>
> 程序已实现多重安全防护（启动恢复、退出归还、EC 防抢夺），但请了解潜在风险后使用。

## 📄 协议

MIT License — 详见 [LICENSE](LICENSE)
