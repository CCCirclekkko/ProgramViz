# ProgramViz 开发者说明

本文档说明如何配置、构建、测试和调试 ProgramViz。项目当前优先支持 macOS；README 中的 VS Code 任务也按 macOS 环境配置。

## 技术栈与目录

- C++ 20
- Qt 6.8 Widgets、Core、Gui、Test
- CMake 3.21 或更高版本，Ninja 生成器
- macOS 应用包（ProgramViz.app）

主要目录：

~~~text
src/
  main.cpp                    进程入口和命令行参数
  mainwindow.*                主窗口、扫描流程、Git 和导出操作
  projectmodel.*              工程节点、扫描器和行数统计
  treemapview.*               树状图布局、绘制和交互
  visualsettings.*            外观设置数据与 HSI 转换
  appearancesettingsdialog.* 外观设置窗口
  hsipalettedialog.*          HSI 调色窗口
tests/
  projectmodel_test.cpp       ProjectScanner 和配色函数测试
gallery/
  ProgramViz-evolution.gif    README 展示用动图
.vscode/
  tasks.json                  配置、构建、运行和清理任务
  launch.json                 LLDB 调试配置
  settings.json               CMake、编译数据库和文件关联
CMakeLists.txt                构建目标和测试注册
CMakePresets.json             Debug/Release preset
~~~

## 环境配置

安装以下工具：

1. Qt 6.8 或更高版本，且包含 Core、Gui、Widgets、Test 模块。
2. CMake 3.21 或更高版本。
3. Ninja。
4. Xcode Command Line Tools；调试时需要 LLDB。
5. Git；只有 Git 工程的分支、历史和演进 GIF 功能会使用它。
6. ffmpeg；只有导出演进 GIF 时需要。

CMakePresets.json 的 base preset 默认使用：

~~~text
CMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
~~~

如果 Qt 安装在其他位置，可以临时覆盖该变量：

~~~sh
cmake --preset debug -DCMAKE_PREFIX_PATH=/path/to/Qt
~~~

也可以复制 CMakePresets.json 为未提交的 CMakeUserPresets.json，为本机维护路径。'.gitignore' 已忽略该文件。

仓库内的 '.vscode/settings.json' 和 '.vscode/tasks.json' 将 CMake 命令固定为 '/usr/local/bin/cmake'。如果本机 CMake 位于其他路径，请修改这两个文件，或直接使用终端中的 'cmake' 命令。VS Code 的 C/C++ 扩展使用 'build/debug/compile_commands.json' 获取头文件和编译参数。

macOS 上可以通过 Homebrew 安装外部工具：

~~~sh
brew install cmake ninja ffmpeg
brew install qt
~~~

安装后请确认 'cmake --version'、'ninja --version'、'ffmpeg -version' 和 'git --version' 均可执行；Qt 的实际安装路径需要与 preset 一致。

## 命令行构建

Debug 构建：

~~~sh
cmake --preset debug
cmake --build --preset debug --parallel
open build/debug/ProgramViz.app
~~~

Release 构建：

~~~sh
cmake --preset release
cmake --build --preset release --parallel
open build/release/ProgramViz.app
~~~

清理某个构建目录：

~~~sh
cmake --build --preset debug --target clean
~~~

两个 preset 都会生成 'compile_commands.json'。如果修改了 Qt 安装路径、生成器或其他 CMake 缓存变量，重新运行对应的 configure 命令。

## 测试

测试目标为 'ProgramVizTests'，当前覆盖行数统计、临时工程扫描、父级汇总和 HSI 配色转换：

~~~sh
cmake --preset debug
cmake --build --preset debug --target ProgramVizTests --parallel
ctest --test-dir build/debug --output-on-failure
~~~

也可以直接运行：

~~~sh
./build/debug/ProgramVizTests
~~~

测试使用 'QTemporaryDir' 创建临时工程，不依赖仓库之外的固定 fixture。新增扫描规则或统计口径时，应同时补充边界用例，例如空文件、末尾无换行、块注释、未知扩展名和被忽略目录。

## VS Code 配置与调试

建议安装仓库 '.vscode/extensions.json' 中推荐的扩展：

- CMake Tools（ms-vscode.cmake-tools）
- C/C++（ms-vscode.cpptools）

打开仓库后，CMake Tools 会使用 'debug' preset。可使用以下入口：

- **Tasks: Run Task → ProgramViz: 运行 Debug**：构建后打开 'build/debug/ProgramViz.app'。
- **Run and Debug → ProgramViz: 调试 Debug**：通过 LLDB 启动 'build/debug/ProgramViz.app/Contents/MacOS/ProgramViz'。
- **Tasks: Run Task → ProgramViz: 构建 Release**：构建 Release 版本。
- **Tasks: Run Task → ProgramViz: 清理构建产物**：清理 Debug 目标。

断点建议放在以下入口：

- 'MainWindow::scanPath'：目录校验、Git 快照选择和扫描结果装载。
- 'ProjectScanner::scan' / 'scanDirectory'：文件过滤和工程树构建。
- 'ProjectScanner::countLines'：行数统计口径。
- 'TreeMapView::rebuildLayout' 及布局相关函数：列宽、节点高度和连接线布局。
- 'MainWindow::exportGitEvolutionGif'：历史读取、帧生成和 ffmpeg 编码三个阶段。

调试 Git 或 GIF 导出时，使用一个包含多个提交且工作区干净的测试工程。Git 历史快照写入 'QTemporaryDir'，程序退出后自动清理，不会修改原工程文件。

## 命令行参数

程序接受一个可选的工程目录参数；省略时使用当前工作目录：

~~~sh
build/debug/ProgramViz.app/Contents/MacOS/ProgramViz /path/to/project
~~~

用于截图和自动化导出的参数：

| 参数 | 说明 |
| --- | --- |
| --screenshot PATH | 启动窗口后保存窗口截图并退出。 |
| --screenshot-gif-dialog PATH | 打开 GIF 导出设置窗口，截取主屏幕后退出，适合生成界面素材。 |
| --export-gif PATH | 无交互导出当前分支的 Git 演进 GIF，完成后退出。需要 Git 工程和 ffmpeg。 |
| --gif-fps N | GIF 帧率；代码将值限制在 1～30，界面预设为 1、2、3、4、5、10、20、30。 |
| --transition-duration SEC | 版本间过渡动画时长，单位为秒。默认 0.5。 |
| --pause-duration SEC | 每个版本的停顿时长，单位为秒。默认 0.5。 |
| --version-duration SEC | 设置每个版本总时长；实现上会用总时长减去过渡时长得到停顿时长。 |
| --gif-resolution SPEC | 分辨率，可用 current、fullscreen、max-height、max-width 或 宽x高。 |
| --gif-fit-height | 对全部历史帧计算适合窗口高度的行高比例。 |
| --gif-no-fit-height | 关闭 GIF 高度自适应。 |
| --gif-expand-names | 提高节点最小高度，展开所有名称。 |
| --gif-collapse-names | 使用当前外观设置的名称高度。 |

示例：

~~~sh
build/debug/ProgramViz.app/Contents/MacOS/ProgramViz \
  /path/to/project \
  --export-gif /tmp/programviz-evolution.gif \
  --gif-fps 10 \
  --transition-duration 0.5 \
  --pause-duration 1.0 \
  --gif-resolution 1280x720 \
  --gif-fit-height \
  --gif-expand-names
~~~

GIF 导出始终以所有历史帧中最宽的布局作为实际画布宽度；'--gif-resolution' 中的宽度是最小宽度。历史读取、动画帧生成和 ffmpeg 编码会分别显示进度。

## 运行时行为与实现约束

### 扫描与统计

'ProjectScanner::defaultOptions()' 定义当前默认扩展名和忽略目录。默认忽略 '.git'、'.svn'、'.hg'、'build'、'dist'、'node_modules'、'.cache' 和 '__pycache__'；符号链接会被跳过。当前这些规则是代码中的默认选项，还没有对应的用户设置界面。

文件节点读取文件内容后统计总行数、有效代码行、空行和基础注释行；目录和工程节点通过 'ProjectNode::aggregate()' 汇总后代数据。未知扩展名不会进入工程树，无法读取的源文件会保留文件节点但行数为零。

### Git

选择 Git 分支或提交后，程序通过 'git ls-tree' 和 'git cat-file --batch' 把可视化文件写入临时快照，再复用同一套扫描器和布局。当前历史菜单显示最多 100 条记录；GIF 导出使用所选分支的完整 'git rev-list --reverse' 历史。Git 命令均通过 'QProcess' 执行，不拼接 shell 命令。

### 外观设置

外观和导出选项通过 'QSettings' 持久化。主要键包括：

~~~text
appearance/theme
appearance/colorScheme
appearance/fontSize
appearance/hue
appearance/saturation
appearance/intensity
appearance/minHeightRatio
appearance/cornerRatio
appearance/columnGap
appearance/siblingGap
appearance/livePreview
export/fps
export/transitionDuration
export/pauseDuration
export/resolution
export/fitHeight
export/expandNames
export/showDate
recentProjects
~~~

应用通过 QApplication::setOrganizationName("ProgramViz") 和 setApplicationName("ProgramViz") 使用平台默认的 Qt 设置存储位置；不要在代码中假定某个固定配置文件路径。

## 修改流程

1. 修改 C++ 或 CMake 文件。
2. 运行 Debug 构建和 'ctest --test-dir build/debug --output-on-failure'。
3. 对界面、布局、导出或 Git 行为进行手工回归。
4. 如果改变用户可见行为，同步更新根目录 [README.md](../README.md)；如果改变配置、调试或命令行行为，同步更新本文档。
5. 将未完成项记录到 [PROGRESS.md](PROGRESS.md)，不要把计划中的功能写成已实现功能。

## 已知限制

- 当前工程以 macOS 为主要目标，Windows/Linux 尚未完成平台验证。
- 扫描在 UI 线程执行，大型工程可能暂时阻塞界面；取消扫描和后台线程仍待实现。
- 源文件扩展名和忽略目录使用编译期默认规则，尚未开放为用户配置。
- GIF 导出依赖外部 ffmpeg，并且只适用于能读取到 Git 历史的工程。
- 项目尚未提供安装包、签名配置或开源许可证。
