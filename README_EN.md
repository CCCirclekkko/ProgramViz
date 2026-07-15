# ProgramViz

中文 | [English](README_EN.md)

[![CI](https://github.com/CCCirclekkko/ProgramViz/actions/workflows/ci.yml/badge.svg)](https://github.com/CCCirclekkko/ProgramViz/actions/workflows/ci.yml)

Current version: `v0.1.0` (macOS)

ProgramViz is a lightweight Qt 6 tool for visualizing project code structure. It scans a selected project, summarizes the file tree and effective code lines, and displays the result as a left-to-right tree map. Git projects can also be viewed by branch or historical revision and exported as evolution GIFs.

## Features

- Recursively scans projects and recognizes common C/C++, Qt, Python, JavaScript/TypeScript, JSON, XML, and Markdown files.
- Displays the file tree with total lines, code lines, blank lines, comment lines, and file sizes.
- Supports hover highlighting, collapsing/expanding, details, and opening files in Finder or VS Code.
- Supports light, dark, and system themes, with adjustable font size, HSI colors, spacing, and minimum node height.
- Supports recent projects, Git branches and revisions, JPG structure images, and Git evolution GIF export.

## Installation and usage

macOS packages are available from [Releases](https://github.com/CCCirclekkko/ProgramViz/releases). The package includes the required Qt libraries.

You can also build from source with Qt 6.8+, CMake 3.21+, and Ninja:

```sh
cmake --preset debug
cmake --build --preset debug
open build/debug/ProgramViz.app
```

- Open the app, or build and run it with the commands above.
- Click “选择” (Select) and choose the project root, or pass the directory path at startup.
- Hover over or click a node to view details; selected files can be opened in Finder or VS Code.
- Use “适应窗口” (Fit Window) and “展开所有名称” (Expand Names) to adjust the layout; use “外观” (Appearance) for theme, font, and colors.
- In a Git project, use “Git 分支” (Git Branch) and “Git 历史” (Git History) to switch revisions.
- Export a JPG or Git evolution GIF. GIF export requires an executable `ffmpeg` installation.
- In VS Code, press `⌘⇧P` and run `Shell Command: Install 'code' command in PATH`, then restart ProgramViz.
- Run `which code` and `code --version` in a terminal to verify the VS Code command is on PATH.

Development, testing, and CLI automation details are in [README_DEV.md](doc/README_DEV.md). The roadmap is in [PROGRESS.md](doc/PROGRESS.md). See [WORKFLOW.md](doc/WORKFLOW.md) for branching, versioning, CI, and release processes.

## TODO

- Improve color schemes and management of custom palettes.
- Add Windows/Linux support and platform-specific file opening.
- Add language support.
- Improve scanning, layout, and GIF export performance for large projects.

## License

ProgramViz is released under the [Apache License 2.0](LICENSE).

ProgramViz uses Qt 6. Qt components in distributed packages are provided under their applicable open-source licenses. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). GIF export requires a user-installed `ffmpeg`; it is not bundled with ProgramViz.
