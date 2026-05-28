# DeepSeek-API-Client-QT

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-6.11.0-green)](https://www.qt.io/)
[![Release](https://img.shields.io/github/v/release/Stan-Vincent/DeepSeek-API-Client-QT)](https://github.com/Stan-Vincent/DeepSeek-API-Client-QT/releases)

一个基于 Qt 6 开发的 DeepSeek API 桌面客户端，支持多会话、流式输出、Markdown 渲染、深色/浅色主题切换等功能。

## ✨ 功能亮点

- 🗂️ **多会话管理** – 新建、重命名、删除对话，历史记录自动保存到 `sessions.json`
- 💬 **流式对话** – 实时显示 AI 回复，支持停止生成
- 🎨 **Markdown 渲染** – 代码高亮、表格、列表、引用等格式完美支持
- 🌗 **深色/浅色主题** – 一键切换，保护视力
- ⚙️ **可配置参数** – API Key、模型、温度、最大 Token、思考模式
- 📤 **导出对话** – 支持导出为 Markdown、HTML 或纯文本
- ⌨️ **快捷键** – 发送(Ctrl+Enter)、新建(Ctrl+N)、删除(Ctrl+D) 等
- 🔒 **本地存储** – 所有对话记录保存在本地，隐私可控

## 🚀 快速入门

### 1️⃣ 获取 API Key
- 访问 [DeepSeek 平台](https://platform.deepseek.com/) 注册账号
- 进入控制台 -> API Keys -> 创建新密钥
- 复制生成的密钥（格式如 `sk-xxxxxxxxxxxxxxxx`）

### 2️⃣ 下载程序
- 前往 [Releases](https://github.com/Stan-Vincent/DeepSeek-API-Client-QT/releases) 页面
- 下载最新版本的 `DeepSeek_Chat_vX.X.X.zip`
- 解压到任意文件夹

### 3️⃣ 放置样式文件（重要！）
- 将解压包中的 `light.txt`、`dark.txt`、`MarkDown.qss` **与主程序 `DeepSeek_Chat.exe` 放在同一目录下**
- 否则程序将无法正常显示界面主题和 Markdown 格式

### 4️⃣ 运行并配置 API Key
- 首次启动程序会弹出“API设置”窗口
- 在 `API Key` 输入框中粘贴你的密钥
- 其他参数（模型、温度等）可按需调整
- 点击 `OK` 保存，即可开始聊天

> **提示**：你也可以随时通过菜单 `设置(S)` → `偏好设置` 修改配置。

## 📦 下载与运行（Release）

如果你不想编译源码，可以直接使用预编译好的可执行文件：

1. 访问 [Releases](https://github.com/Stan-Vincent/DeepSeek-API-Client-QT/releases)
2. 下载 `DeepSeek_Chat_vX.X.X.zip`
3. 解压后，确认以下文件在同一目录下：
   - `DeepSeek_Chat.exe`
   - `light.txt` / `dark.txt`
   - `MarkDown.qss`
4. 双击 `DeepSeek_Chat.exe` 启动程序
5. 如果提示缺少 `VCRUNTIME140.dll` 等运行时库，请安装 [Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)

## 🛠️ 从源码构建（Build from Source）

### 环境要求
- Qt 6.4 或更高版本（推荐 Qt 6.11.0）
- CMake 3.16+
- 支持 C++17 的编译器（MSVC 2022 / MinGW 64-bit）

### 编译步骤
```bash
git clone https://github.com/Stan-Vincent/DeepSeek-API-Client-QT.git
cd DeepSeek-API-Client-QT
mkdir build && cd build
cmake ..
cmake --build . --config Release
