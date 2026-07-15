# 稳定性与可移植性审阅

审阅基线：当前 main 分支，CMake 项目版本 0.1.0。

## 审阅结论

当前代码适合作为 macOS-only 的首个公开早期版本发布。没有发现会阻止 macOS 本地运行和基础测试的明显稳定性问题；Windows/Linux 不属于当前发布范围。

当前发布定位建议为：

> Early development release：功能可用，统计规则、跨平台体验和性能仍会继续改进。

## 已验证项目

| 项目 | 结果 | 说明 |
| --- | --- | --- |
| Debug 配置与构建 | 通过 | 使用 CMake Debug preset。 |
| Debug CTest | 通过 | 当前 1 个测试目标通过。 |
| Release 配置与构建 | 通过 | 使用 CMake Release preset；本地构建目录已是最新。 |
| Release CTest | 通过 | 当前 1 个测试目标通过。 |
| CMake/Git 标签版本校验 | 通过 | 0.1.0 与 CMakeLists.txt 一致。 |
| GitHub Actions YAML 语法 | 通过 | CI、Release 和 Issue Form 文件可被 YAML 解析。 |
| GitHub macOS 真实构建 | 待 GitHub 验证 | 本地已完成 Debug/Release 构建，仍需 Runner 验证。 |
| macOS DMG 启动 | 待 GitHub 验证 | 尚未在真实 Runner 上完成 DMG 验收。 |

## 稳定性观察

### 已有保护

- 文件扫描会校验目录存在性，并跳过符号链接和默认忽略目录。
- Git 历史和 GIF 导出使用临时目录，不会直接写入用户工程。
- Git、ffmpeg 和 VS Code 通过 QProcess 传递参数，不拼接 shell 命令。
- 外部进程失败会显示状态信息，Git 历史读取失败会恢复当前工作区。
- 目录汇总、空文件和基础行数统计已有单元测试。
- 导出分为读取历史、生成帧、编码 GIF 三个阶段，并支持取消进度对话框。

### 发布时应明确的限制

- 扫描和部分 Git 操作在 UI 线程中同步执行；大型工程可能暂时阻塞界面。
- 行数统计是轻量规则统计，不是完整语言解析器；复杂预处理、混合编码和特殊注释可能产生近似结果。
- 未识别编码或读取失败的文件可能保留节点，但统计行数不完整。
- 大型工程和很长 Git 历史会消耗较多内存、磁盘临时空间和 GIF 生成时间。
- GIF 导出依赖用户环境中的 Git 和 ffmpeg。
- UI 自动化、布局算法、大规模性能和外部程序联动测试目前覆盖有限。

## 可移植性观察

### 已采用的跨平台方式

- 构建使用 CMake、Ninja 和 Qt 6，不依赖 macOS 专属构建系统。
- 文件、路径、临时目录、日期和进程调用使用 Qt API。
- Finder 打开逻辑使用 macOS 的 open；其他平台使用 QDesktopServices。
- 当前 CI 和 Release 工作流只覆盖 macOS；后续跨平台开发时再扩展矩阵。

### 需要注意的环境差异

- CMakePresets.json 的本机 preset 默认指向 /opt/homebrew/opt/qt，适合当前 macOS Homebrew 环境，不适合直接作为 Windows/Linux 本地 preset。
- .vscode/tasks.json 使用 /usr/local/bin/cmake，其他开发者应使用终端或修改本机 VS Code 设置。
- VS Code 打开文件依赖 PATH 中的 code 命令；没有 VS Code 时不影响扫描和可视化。
- Git 和 ffmpeg 必须在 PATH 中；macOS 额外尝试 Homebrew 常见路径。
- macOS、Windows 和 Linux 的字体、可用屏幕尺寸、文件创建时间和默认主题可能导致布局或日期显示略有不同。
- Release 工作流依赖 macdeployqt；该步骤尚未在本地完整复现。

## 发布前建议

1. 先让 macOS CI 的 Debug 构建和测试通过。
2. 从一个带多个提交的真实 Git 工程手工验证分支、历史、JPG 和 GIF。
3. 在 Release workflow 成功后下载 macOS DMG 并启动一次。
4. 检查安装包是否包含 Qt 运行库和必要插件。
5. 再配置代码签名、macOS notarization 和正式许可证。
6. 将验证结果和已知问题写入 GitHub Release 说明。

## 后续改进优先级

1. 将扫描和 Git 快照读取移到工作线程，避免界面阻塞。
2. 增加布局、GIF 导出和跨平台路径行为测试。
3. 增加扫描失败、编码异常、超大文件和临时磁盘不足的诊断信息。
4. 为 CMake preset 提供跨平台用户 preset 示例。
5. 为安装包增加版本信息、图标、签名和自动更新策略。
