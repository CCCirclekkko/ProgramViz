# Release Checklist

适用于 vX.Y.Z 正式发布或预发布版本。

## 版本与源代码

[ ] CMakeLists.txt 的 project version 已更新。
[ ] Git tag 使用 vX.Y.Z，并与 CMake 版本一致。
[ ] CHANGELOG.md 已记录用户可见变化。
[ ] README、README_DEV 和 WORKFLOW 中没有过时的命令或功能描述。
[ ] 已确认许可证文件存在，或明确标记为未授权发布。

## 本地验证

[ ] Debug 构建通过。
[ ] Release 构建通过。
[ ] Debug CTest 通过。
[ ] Release CTest 通过。
[ ] git diff --check 通过。
[ ] 使用一个小型 Git 工程完成扫描、分支、历史和导出手工检查。
[ ] 使用一个大型工程确认没有崩溃、数据覆盖或明显错误。

## GitHub Actions

[ ] main 分支 CI 在 macOS 上通过。
[ ] Release workflow 的版本校验通过。
[ ] macOS DMG 构建成功。
[ ] macOS DMG 能启动，并包含 Qt 运行时依赖。
[ ] Release 页面已生成正确的变更说明和下载文件。

## 发布后

[ ] 将 Draft Release 改为正式发布。
[ ] 在 README 中确认最新版本和下载入口。
[ ] 记录用户反馈和首批问题。
[ ] 将未解决问题转为 GitHub Issue，并更新 doc/PROGRESS.md。
