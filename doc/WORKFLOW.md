# ProgramViz 开源协作与发布工作流

## 1. 分支模型

仓库采用轻量的 Git Flow：

- main：始终保持可构建、可测试、可发布；只接受 Pull Request。
- feature/<topic>：新功能、跨平台适配和重构。
- fix/<topic>：普通缺陷修复。
- release/v<MAJOR>.<MINOR>.<PATCH>：发布候选验证；只接受版本修复和文档更新。
- hotfix/v<MAJOR>.<MINOR>.<PATCH>：从 main 紧急修复线上版本。

小规模贡献可以直接从 main 创建 feature 或 fix 分支；不再长期维护 develop 分支，避免形成第二个不稳定主线。

建议命令：

~~~sh
git switch main
git pull --ff-only
git switch -c feature/short-description
~~~

合并前必须通过 CI 和代码审查。禁止向 main 强制推送；GitHub 仓库启用 main 分支保护后，要求 Pull Request、CI 检查和至少一名审查者。

## 2. 版本规则

版本遵循 Semantic Versioning：

- MAJOR：不兼容的用户界面、命令行或数据行为变化。
- MINOR：向后兼容的新功能。
- PATCH：向后兼容的修复、文档和打包修正。

当前版本源头是 CMakeLists.txt 中的 project(ProgramViz VERSION ...)。发布标签必须与该版本一致，例如：

~~~text
CMake project version: 0.2.0
Git tag: v0.2.0
GitHub Release: 0.2.0
~~~

版本发布流程：

1. 从 main 创建 release/vX.Y.Z。
2. 修改 CMakeLists.txt 的项目版本、CHANGELOG.md 和必要的用户/开发文档。
3. 在 macOS CI 验证并手工检查 DMG 安装包。
4. 合并 release 分支到 main。
5. 在 main 上创建并推送 vX.Y.Z 标签：

~~~sh
git switch main
git pull --ff-only
git tag -a v0.2.0 -m "ProgramViz v0.2.0"
git push origin v0.2.0
~~~

6. GitHub Actions 根据 v*.*.* 标签构建 macOS DMG，并创建 GitHub Release 草稿。
7. 检查 Release 说明、安装包和校验值后，在 GitHub Release 页面点击 Publish release。

## 3. 日常开发检查

提交前至少运行：

~~~sh
cmake --preset debug
cmake --build --preset debug --parallel
ctest --test-dir build/debug --output-on-failure
git diff --check
~~~

当前版本只验证 macOS。跨平台代码应避免直接依赖 macOS API；未来扩展 Windows/Linux 时，再将平台相关行为集中在条件编译或独立 helper 中，并扩展 CI 矩阵。

## 4. Pull Request 规则

Pull Request 应包含：

- 变更目的和用户可见影响。
- 测试命令及结果。
- 界面变更前后截图或 GIF。
- 是否改变命令行、配置、统计口径或文件格式。
- 是否需要更新 README、README_DEV 或 PROGRESS。

提交信息建议使用 Conventional Commits，例如：

~~~text
feat: add project zoom controls
fix: preserve layout width during GIF export
docs: update release workflow
ci: test Qt build on Windows
~~~

## 5. GitHub 仓库设置清单

创建 GitHub 仓库后完成以下设置：

- 将 main 设置为默认分支并启用分支保护。
- 要求 Pull Request、CI/build 检查和线性历史（如团队习惯允许）。
- 开启 Discussions，用于用户问答和使用经验。
- 开启 Issues，使用仓库内的 Bug 和功能请求模板。
- 为 v*.*.* 标签配置 Release 工作流。
- 添加仓库描述、Topics 和项目主页。
- 将 `.github/ISSUE_TEMPLATE/config.yml` 中的 `OWNER/REPOSITORY` 替换为真实仓库路径。
- 补充 LICENSE；没有许可证时，代码不应被当作可自由复用的开源软件。
- 如果未来使用签名证书、Homebrew、winget 或 Flathub，使用 GitHub Actions Secrets 管理凭据，不要提交到仓库。

发布前请按 [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md) 操作，并参考 [STABILITY_PORTABILITY_REVIEW.md](STABILITY_PORTABILITY_REVIEW.md) 中的已知限制和跨平台风险。
