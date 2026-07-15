# ProgramViz

[English](README_EN.md) | 中文

[![CI](https://github.com/CCCirclekkko/ProgramViz/actions/workflows/ci.yml/badge.svg)](https://github.com/CCCirclekkko/ProgramViz/actions/workflows/ci.yml)

当前版本：`v0.1.0`（macOS）

ProgramViz 是一个基于 Qt 6 的轻量工程代码结构可视化工具。它扫描用户选择的工程目录，将文件树、目录层级和有效代码行数汇总为从左到右展开的树状图，帮助快速定位工程中体量较大的模块和文件。对于 Git 工程，还可以查看分支与历史版本，并导出演进 GIF。

![ProgramViz Git 演进示例](gallery/ProgramViz-evolution.gif)

## 功能

- 递归扫描工程目录，识别常见的 C/C++、Qt、Python、JavaScript/TypeScript、JSON、XML、Markdown 等源代码文件。
- 按文件树展示工程结构，并汇总总行数、有效代码行数、空行、注释行和文件大小。
- 支持节点悬停高亮、折叠/展开、详情查看，以及在 Finder 或 VS Code 中打开。
- 支持浅色、深色和跟随系统主题，可调整字号、HSI 配色、间距和节点最小高度。
- 支持最近工程目录、Git 分支与历史版本，以及 JPG 工程结构图和 Git 演进 GIF 导出。

## 安装与使用

macOS 安装包可从 [Releases](https://github.com/CCCirclekkko/ProgramViz/releases) 获取。当前仅提供 macOS 版本，安装包已包含运行所需的 Qt 库。

也可以从源码构建。建议使用 Qt 6.8 或更高版本、CMake 3.21 或更高版本和 Ninja：

```sh
cmake --preset debug
cmake --build --preset debug
open build/debug/ProgramViz.app
```

- 从 Releases 下载并打开应用，或执行上面的 CMake 命令构建后运行。
- 点击“选择”，选择工程根目录；也可以在启动时传入目录路径。
- 将鼠标悬停或点击节点查看详情；点击文件后可在 Finder 或 VS Code 中打开。
- 使用“适应窗口”和“展开所有名称”调整布局，使用“外观”修改主题、字号和配色。
- Git 工程可通过“Git 分支”和“Git 历史”切换分支或历史提交。
- 从导出功能选择 JPG 或 Git 演进 GIF；GIF 导出前请安装并确保 `ffmpeg` 可执行。
- 在 VS Code 中按 `⌘⇧P`，执行 `Shell Command: Install 'code' command in PATH`，然后重启 ProgramViz。
- 在终端执行 `which code` 和 `code --version`，确认 VS Code 命令已加入 PATH。

开发、调试、测试和命令行自动化说明见 [README_DEV.md](doc/README_DEV.md)，开发路线和待办事项见 [PROGRESS.md](doc/PROGRESS.md)。分支、版本、CI 和发布流程见 [WORKFLOW.md](doc/WORKFLOW.md)；发布前检查见 [RELEASE_CHECKLIST.md](doc/RELEASE_CHECKLIST.md) 和 [STABILITY_PORTABILITY_REVIEW.md](doc/STABILITY_PORTABILITY_REVIEW.md)；贡献代码请阅读 [CONTRIBUTING.md](CONTRIBUTING.md)。

## TODO

- 完善上色方案和用户自定义配色的管理。
- 增加 Windows/Linux 跨平台支持和平台相关的文件打开方式。
- 添加语言支持。
- 优化大型工程扫描、布局和 GIF 导出性能。

## 许可

ProgramViz 自身代码采用 [Apache License 2.0](LICENSE) 发布。

ProgramViz 使用 Qt 6。发布包中的 Qt 组件按照其适用的开源许可证分发，相关版权和许可证信息见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。导出 Git 演进 GIF 时需要用户自行安装 `ffmpeg`；该工具不随 ProgramViz 安装包发布。
