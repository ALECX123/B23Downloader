# B23Downloader

B 站（哔哩哔哩）视频/直播/漫画下载器，基于 Qt6 开发，支持 UGC 视频、PGC 剧集（番剧/电影/纪录片）、PUGV 课程、直播录像与漫画下载。

[English](./README.en.md) | 简体中文

---

## 功能特性

- **多内容类型支持**
  - UGC 普通视频（`BV`/`av` 链接）
  - PGC 剧集（番剧、电影、纪录片等，支持 `ss`/`ep`/`md` 链接）
  - PUGV 课程
  - 直播录制（FLV 实时修复时间戳并注入关键帧索引）
  - 漫画下载
- **扫码登录**：通过 B 站 App 扫码完成登录，Cookie 持久化保存
- **清晰度选择**：自动获取可选清晰度列表（4K / 1080P / 720P 等）
- **批量下载**：支持番剧分集、视频分 P 批量勾选
- **MP4 合并**：调用 `ffmpeg.exe` 将 m4s 视频流与音频流合并为 MP4
- **任务管理**：下载队列、启动/停止/删除、实时速度与剩余时间统计
- **断点续传**：任务列表持久化，重启后可恢复

## 下载

前往 [Releases](https://github.com/ALECX123/B23Downloader/releases) 下载最新版本压缩包，解压后运行 `B23Downloader.exe` 即可。

> 运行目录需自带 `ffmpeg.exe`（用于视频合并），发行包内已包含。

## 使用说明

1. 启动程序后点击右上角头像按钮，使用 B 站 App 扫码登录。
2. 将 B 站视频/番剧/直播/漫画链接粘贴到地址栏，点击下载按钮。
3. 在弹出的对话框中选择清晰度与需要下载的分集，设置保存路径，点击确定。
4. 在主界面任务列表中查看进度、暂停或删除任务。

支持的链接形式（示例）：
- 视频：`https://www.bilibili.com/video/BV1xx411c7mD`
- 番剧：`https://www.bilibili.com/bangumi/play/ss12345`
- 直播：`https://live.bilibili.com/12345`
- 漫画：`https://manga.bilibili.com/detail/mc12345`

## 从源码编译

### 依赖

- **Qt 6**（组件：Core、Gui、Widgets、Network）
- **C++17** 编译器（MSVC / GCC / Clang）
- **CMake** ≥ 3.16
- **OpenSSL**（Qt Network TLS 所需）
- **ffmpeg.exe**（运行期依赖，放在可执行文件目录或 PATH 中）

### 编译步骤

```bash
git clone https://github.com/ALECX123/B23Downloader.git
cd B23Downloader
cmake -B build -S B23Downloader
cmake --build build --config Release
```

### Windows 部署

使用 `windeployqt` 收集运行时依赖，并将 `ffmpeg.exe` 拷贝到输出目录：

```powershell
windeployqt --release --no-translations build\Release\B23Downloader.exe
copy ffmpeg.exe build\Release\
```

## 项目结构

```
B23Downloader/
├── B23Downloader/            # 源码目录
│   ├── main.cpp              # 程序入口（Windows 单实例控制）
│   ├── MainWindow.{h,cpp}    # 主窗口
│   ├── LoginDialog.{h,cpp}   # 扫码登录对话框
│   ├── DownloadDialog.{h,cpp}# URL 解析与下载选项对话框
│   ├── Extractor.{h,cpp}     # URL 解析与内容信息提取
│   ├── DownloadTask.{h,cpp}  # 下载任务（UGC/PGC/PUGV/Live/Comic）
│   ├── Flv.{h,cpp}           # FLV 解析与直播 remux
│   ├── Network.{h,cpp}       # B 站网络请求封装
│   ├── Settings.{h,cpp}      # 配置与 Cookie 持久化
│   ├── TaskTable.{h,cpp}     # 任务列表 UI
│   ├── QrCode.{h,cpp}        # 二维码生成
│   ├── AboutWidget.{h,cpp}   # 关于页面
│   ├── MyTabWidget.{h,cpp}   # 自定义 Tab 控件
│   ├── utils.{h,cpp}         # 通用工具函数
│   ├── icons.qrc             # 图标资源
│   ├── icons/                # 图标文件
│   ├── B23Downloader_resource.rc  # Windows 资源（图标/版本）
│   └── CMakeLists.txt        # CMake 构建脚本
└── LICENSE                   # GPL-3.0
```

### 核心模块

| 模块 | 职责 |
|------|------|
| `DownloadTask` | 抽象下载任务基类及 UGC/PGC/PUGV/Live/Comic 子类，负责请求播放地址、下载流并合并 |
| `Extractor` | 根据 URL 识别内容类型并拉取分集列表 |
| `Network::Bili` | 统一的 B 站 HTTP 请求封装（Referer/UA/Cookie） |
| `Flv` | FLV 容器解析、直播流时间戳重置与关键帧索引注入 |
| `Settings` | 基于 `QSettings` 的配置存储与 Cookie 管理 |
| `TaskTable` | 下载任务表格视图，支持启动/停止/删除与速度统计 |

## License

本项目基于 [GPL-3.0](./LICENSE) 协议开源。

## 致谢

- [Qt](https://www.qt.io/) — 跨平台应用开发框架
- [FFmpeg](https://ffmpeg.org/) — 多媒体处理
- 哔哩哔哩用户提供的内容资源

---

> 本项目仅供学习交流使用，不得用于商业用途。下载的内容版权归原所有者所有，请在 24 小时内删除。
