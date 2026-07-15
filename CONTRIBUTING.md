# 参与贡献

感谢参与 ProgramViz。当前公开发布目标是 macOS；后续跨平台适配会在单独的工作流和版本中进行。

## 开始之前

- 阅读 [README.md](README.md) 了解用户功能。
- 阅读 [doc/README_DEV.md](doc/README_DEV.md) 了解构建、测试和调试。
- 阅读 [doc/WORKFLOW.md](doc/WORKFLOW.md) 了解分支、版本和发布规则。
- 对照 [doc/PROGRESS.md](doc/PROGRESS.md) 选择尚未完成的工作。

## 提交代码

1. 从最新的 main 创建 feature/<topic> 或 fix/<topic> 分支。
2. 保持提交聚焦，避免把格式化、重命名和功能修改混在一起。
3. 为行为变化补充测试；界面变化附截图或 GIF。
4. 运行 Debug 构建、CTest 和 git diff --check。
5. 创建 Pull Request，并填写模板中的影响范围、验证方式和文档更新情况。

建议的本地检查：

~~~sh
cmake --preset debug
cmake --build --preset debug --parallel
ctest --test-dir build/debug --output-on-failure
git diff --check
~~~

## 报告问题

Bug 请提供操作系统、ProgramViz 版本、复现步骤、预期/实际结果，以及相关日志或截图。功能建议请说明使用场景和希望解决的问题。安全问题请按照 [SECURITY.md](SECURITY.md) 的说明处理。
