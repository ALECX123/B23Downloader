# B23Downloader

A downloader for Bilibili videos, live streams and manga, built with Qt6. Supports UGC videos, PGC series (anime/movies/documentaries), PUGV courses, live recording and manga downloads.

<p align="center">
  <img src="./B23Downloader/icons/icon-96x96.png" alt="icon">
</p>

English | [简体中文](./README.md)

---

## Features

- **Multiple content types**
  - UGC user videos (`BV`/`av` links)
  - PGC series (anime, movies, documentaries; `ss`/`ep`/`md` links)
  - PUGV courses
  - Live recording (real-time FLV timestamp fix and keyframe index injection)
  - Manga downloads
- **QR Login** — scan with the Bilibili App to sign in; cookies are persisted
- **Quality selection** — auto-fetches available resolutions (4K / 1080P / 720P, etc.)
- **Batch download** — multi-episode selection for series and multi-part videos
- **MP4 merging** — invokes `ffmpeg.exe` to merge m4s video/audio streams into MP4
- **Task management** — download queue with start/stop/remove and real-time speed / ETA
- **Resumable tasks** — task list is persisted and restored on restart

## Download

Go to [Releases](https://github.com/ALECX123/B23Downloader/releases), download the latest archive, extract it and run `B23Downloader.exe`.

> `ffmpeg.exe` (used for stream merging) must be placed alongside the executable; it is already bundled in the release package.

## Usage Guide

1. Launch the app and click the avatar button in the top-right corner, then scan the QR code with the Bilibili App to sign in.
2. Paste a Bilibili video / series / live / manga URL into the address bar and click the download button.
3. In the dialog, pick the desired quality and episodes, choose a save folder, and click OK.
4. Track progress, pause or remove tasks in the main task list.

Supported URL examples:
- Video: `https://www.bilibili.com/video/BV1xx411c7mD`
- Anime/Series: `https://www.bilibili.com/bangumi/play/ss12345`
- Live: `https://live.bilibili.com/12345`
- Manga: `https://manga.bilibili.com/detail/mc12345`

## Building from Source

### Dependencies

- **Qt 6** (components: Core, Gui, Widgets, Network)
- **C++17** compiler (MSVC / GCC / Clang)
- **CMake** ≥ 3.16
- **OpenSSL** (required by Qt Network TLS)
- **ffmpeg.exe** (runtime dependency; place in the executable directory or PATH)

### Build Steps

```bash
git clone https://github.com/ALECX123/B23Downloader.git
cd B23Downloader
cmake -B build -S B23Downloader
cmake --build build --config Release
```

### Windows Deployment

Use `windeployqt` to collect runtime dependencies, then copy `ffmpeg.exe` to the output directory:

```powershell
windeployqt --release --no-translations build\Release\B23Downloader.exe
copy ffmpeg.exe build\Release\
```

## Project Architecture

```
B23Downloader/
├── B23Downloader/            # source directory
│   ├── main.cpp              # entry point (single-instance on Windows)
│   ├── MainWindow.{h,cpp}    # main window
│   ├── LoginDialog.{h,cpp}   # QR login dialog
│   ├── DownloadDialog.{h,cpp}# URL parsing & download options dialog
│   ├── Extractor.{h,cpp}     # URL parsing and content info extraction
│   ├── DownloadTask.{h,cpp}  # download tasks (UGC/PGC/PUGV/Live/Comic)
│   ├── Flv.{h,cpp}           # FLV parsing and live remux
│   ├── Network.{h,cpp}       # Bilibili HTTP request wrappers
│   ├── Settings.{h,cpp}      # config & cookie persistence
│   ├── TaskTable.{h,cpp}     # task list UI
│   ├── QrCode.{h,cpp}        # QR code generator
│   ├── AboutWidget.{h,cpp}   # about page
│   ├── MyTabWidget.{h,cpp}   # custom tab widget
│   ├── utils.{h,cpp}         # utility helpers
│   ├── icons.qrc             # icon resources
│   ├── icons/                # icon files
│   ├── B23Downloader_resource.rc  # Windows resource (icon/version)
│   └── CMakeLists.txt        # CMake build script
└── LICENSE                   # GPL-3.0
```

### Core Modules

| Module | Responsibility |
|--------|----------------|
| `DownloadTask` | Abstract download task base and UGC/PGC/PUGV/Live/Comic subclasses; requests play URL, downloads streams and merges them |
| `Extractor` | Identifies content type from a URL and fetches the episode list |
| `Network::Bili` | Unified Bilibili HTTP request wrapper (Referer/UA/Cookie) |
| `Flv` | FLV container parsing, live stream timestamp reset and keyframe index injection |
| `Settings` | `QSettings`-based config storage and cookie management |
| `TaskTable` | Download task table view with start/stop/remove and speed stats |

## License

This project is open-sourced under the [GPL-3.0](./LICENSE) license.

## Acknowledgments

- Derived from [vooidzero/B23Downloader](https://github.com/vooidzero/B23Downloader)
- [Qt](https://www.qt.io/) — cross-platform application framework
- [FFmpeg](https://ffmpeg.org/) — multimedia processing
- Bilibili content providers

---