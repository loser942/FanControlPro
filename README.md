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
| 🛡️ EC 防抢夺 | 独立每秒刷新，对抗 Clevo EC 固件看门狗 |
| 💥 崩溃安全 | 异常退出后启动时自动归还 BIOS 控制权 |
| 🎨 Win11 UI | 深色主题、温度进度条、实时 RPM 显示 |
| 🔔 温度告警 | CPU/GPU 超温托盘气泡通知 |
| 🚀 开机自启 | 注册表 + 任务计划双保险 |
| ⚡ 预设切换 | 静音/性能/平衡一键切换 |

## 🖥️ 适用机型

- **硬件**：蓝天 (Clevo) 模具笔记本
- **测试配置**：i5-13500H + RTX 4060 + Windows 11

## 📋 版本历史

| 版本 | 改进 |
|------|------|
| v1 | 初始重构：三种控制模式 + Win11 UI |
| v2 | 14 项崩溃/逻辑修复 |
| v3 | 6 项并发安全 |
| v4 | 平滑风扇过渡 |
| v5 | 崩溃安全退出 |
| v6 | Clevo EC 防抢夺 |
| **v7** | **4 Bug + 3 风险修复** |

## 🔧 构建与使用

1. **Visual Studio 2022**（含 C++ MFC 支持）打开 `FanControlPro.sln`
2. 编译生成 `FanControlPro.exe`
3. 从 [Releases](../../releases) 下载 `MyFanControl-v1.0.zip`（运行时依赖）
4. 解压后放入编译输出目录（与 `FanControlPro.exe` 同级）
5. 运行 `NTPortDrvSetup.exe` 安装驱动

> ⚠️ **重要提示**：Releases 中的 Zip 包仅包含运行时 DLL 依赖，**不含可执行程序**，必须先编译本项目。

## 📄 协议

MIT License