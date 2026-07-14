#include "projectmodel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QSet>

#include <algorithm>

qint64 ProjectNode::weight() const
{
    if (kind == ProjectNodeKind::File)
        return std::max<qint64>(1, lines.code);

    qint64 total = 0;
    for (const auto &child : children)
        total += child->weight();
    return std::max<qint64>(1, total);
}

bool ProjectNode::isFolder() const
{
    return kind != ProjectNodeKind::File;
}

int ProjectNode::fileCount() const
{
    if (kind == ProjectNodeKind::File)
        return 1;

    int total = 0;
    for (const auto &child : children)
        total += child->fileCount();
    return total;
}

int ProjectNode::folderCount() const
{
    if (kind == ProjectNodeKind::File)
        return 0;

    int total = kind == ProjectNodeKind::Folder ? 1 : 0;
    for (const auto &child : children)
        total += child->folderCount();
    return total;
}

void ProjectNode::aggregate()
{
    if (kind == ProjectNodeKind::File)
        return;

    lines = {};
    sizeBytes = 0;
    for (const auto &child : children) {
        child->aggregate();
        lines.total += child->lines.total;
        lines.code += child->lines.code;
        lines.blank += child->lines.blank;
        lines.comment += child->lines.comment;
        sizeBytes += child->sizeBytes;
    }
}

ProjectScanner::Options ProjectScanner::defaultOptions()
{
    Options options;
    options.extensions = {
        QStringLiteral("c"), QStringLiteral("h"), QStringLiteral("cc"),
        QStringLiteral("cpp"), QStringLiteral("cxx"), QStringLiteral("hh"),
        QStringLiteral("hpp"), QStringLiteral("hxx"), QStringLiteral("qml"),
        QStringLiteral("ui"), QStringLiteral("qrc"), QStringLiteral("py"),
        QStringLiteral("js"), QStringLiteral("jsx"), QStringLiteral("ts"),
        QStringLiteral("tsx"), QStringLiteral("java"), QStringLiteral("kt"),
        QStringLiteral("kts"), QStringLiteral("rs"), QStringLiteral("go"),
        QStringLiteral("swift"), QStringLiteral("cs"), QStringLiteral("rb"),
        QStringLiteral("php"), QStringLiteral("sh"), QStringLiteral("bash"),
        QStringLiteral("zsh"), QStringLiteral("cmake"), QStringLiteral("json"),
        QStringLiteral("xml"), QStringLiteral("md"), QStringLiteral("markdown"),
    };
    options.ignoredDirectoryNames = {
        QStringLiteral(".git"), QStringLiteral(".svn"), QStringLiteral(".hg"),
        QStringLiteral("build"), QStringLiteral("dist"), QStringLiteral("node_modules"),
        QStringLiteral(".cache"), QStringLiteral("__pycache__"),
    };
    return options;
}

std::unique_ptr<ProjectNode> ProjectScanner::scan(const QString &rootPath,
                                                  const Options &options)
{
    QFileInfo rootInfo(rootPath);
    if (!rootInfo.exists() || !rootInfo.isDir())
        return nullptr;

    auto root = std::make_unique<ProjectNode>();
    root->kind = ProjectNodeKind::Project;
    root->name = rootInfo.fileName().isEmpty() ? rootInfo.absoluteFilePath() : rootInfo.fileName();
    root->path = rootInfo.absoluteFilePath();
    root->createdAt = rootInfo.birthTime();
    root->modifiedAt = rootInfo.lastModified();
    scanDirectory(*root, options);
    root->aggregate();
    return root;
}

LineMetrics ProjectScanner::countLines(const QByteArray &contents, const QString &extension)
{
    LineMetrics metrics;
    if (contents.isEmpty())
        return metrics;

    QList<QByteArray> lines = contents.split('\n');
    if (contents.endsWith('\n'))
        lines.removeLast();
    metrics.total = lines.size();
    const QString ext = extension.toLower();
    bool inBlockComment = false;
    for (QByteArray line : lines) {
        if (line.endsWith('\r'))
            line.chop(1);
        const QByteArray trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            ++metrics.blank;
            continue;
        }

        bool comment = inBlockComment;
        int index = 0;
        while (index < trimmed.size()) {
            if (inBlockComment) {
                const int end = trimmed.indexOf("*/", index);
                if (end < 0) {
                    index = trimmed.size();
                    break;
                }
                inBlockComment = false;
                index = end + 2;
                while (index < trimmed.size()) {
                    const char character = trimmed.at(index);
                    if (character != ' ' && character != '\t' && character != '\r')
                        break;
                    ++index;
                }
                if (index < trimmed.size())
                    comment = false;
                continue;
            }

            const int blockStart = trimmed.indexOf("/*", index);
            const int slashStart = trimmed.indexOf("//", index);
            if (slashStart >= 0 && (blockStart < 0 || slashStart < blockStart)) {
                if (slashStart == 0 || trimmed.left(slashStart).trimmed().isEmpty())
                    comment = true;
                break;
            }
            if (blockStart < 0)
                break;
            if (blockStart == 0 || trimmed.left(blockStart).trimmed().isEmpty())
                comment = true;
            const int blockEnd = trimmed.indexOf("*/", blockStart + 2);
            if (blockEnd < 0) {
                inBlockComment = true;
                break;
            }
            index = blockEnd + 2;
        }

        if (ext == QStringLiteral("xml") || ext == QStringLiteral("ui"))
            comment = comment || trimmed.startsWith("<!--");
        if (ext == QStringLiteral("py") || ext == QStringLiteral("rb")
            || ext == QStringLiteral("sh") || ext == QStringLiteral("bash")
            || ext == QStringLiteral("zsh") || ext == QStringLiteral("yaml")) {
            comment = comment || trimmed.startsWith('#') && !trimmed.startsWith("#!");
        }
        if (ext == QStringLiteral("md") || ext == QStringLiteral("markdown"))
            comment = false;

        if (comment)
            ++metrics.comment;
        else
            ++metrics.code;
    }
    return metrics;
}

void ProjectScanner::scanDirectory(ProjectNode &parent, const Options &options)
{
    QDir directory(parent.path);
    const QFileInfoList entries = directory.entryInfoList(
        QDir::NoDotAndDotDot | QDir::AllEntries,
        QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);

    for (const QFileInfo &info : entries) {
        if (info.isSymLink())
            continue;
        if (info.isDir()) {
            if (options.ignoredDirectoryNames.contains(info.fileName(), Qt::CaseSensitive))
                continue;
            auto child = std::make_unique<ProjectNode>();
            child->kind = ProjectNodeKind::Folder;
            child->name = info.fileName();
            child->path = info.absoluteFilePath();
            child->createdAt = info.birthTime();
            child->modifiedAt = info.lastModified();
            child->parent = &parent;
            child->depth = parent.depth + 1;
            scanDirectory(*child, options);
            if (!child->children.empty())
                parent.children.push_back(std::move(child));
            continue;
        }

        if (!info.isFile() || !isSourceFile(info.fileName(), options))
            continue;

        auto child = std::make_unique<ProjectNode>();
        child->kind = ProjectNodeKind::File;
        child->name = info.fileName();
        child->path = info.absoluteFilePath();
        child->language = info.fileName() == QStringLiteral("CMakeLists.txt")
                              ? QStringLiteral("CMake")
                              : languageForExtension(info.suffix());
        child->sizeBytes = info.size();
        child->createdAt = info.birthTime();
        child->modifiedAt = info.lastModified();
        child->parent = &parent;
        child->depth = parent.depth + 1;

        QFile file(info.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly))
            child->lines = countLines(file.readAll(), info.suffix());
        parent.children.push_back(std::move(child));
    }
}

bool ProjectScanner::isSourceFile(const QString &fileName, const Options &options)
{
    if (fileName == QStringLiteral("CMakeLists.txt")
        || fileName == QStringLiteral("Makefile")
        || fileName == QStringLiteral("GNUmakefile")
        || fileName == QStringLiteral("meson.build")) {
        return true;
    }
    const QString extension = QFileInfo(fileName).suffix().toLower();
    return !extension.isEmpty() && options.extensions.contains(extension, Qt::CaseInsensitive);
}

QString ProjectScanner::languageForExtension(const QString &extension)
{
    const QString ext = extension.toLower();
    if (ext == QStringLiteral("cpp") || ext == QStringLiteral("cc") || ext == QStringLiteral("cxx"))
        return QStringLiteral("C++");
    if (ext == QStringLiteral("h") || ext == QStringLiteral("hh") || ext == QStringLiteral("hpp")
        || ext == QStringLiteral("hxx"))
        return QStringLiteral("C/C++ Header");
    if (ext == QStringLiteral("py"))
        return QStringLiteral("Python");
    if (ext == QStringLiteral("js") || ext == QStringLiteral("jsx"))
        return QStringLiteral("JavaScript");
    if (ext == QStringLiteral("ts") || ext == QStringLiteral("tsx"))
        return QStringLiteral("TypeScript");
    if (ext == QStringLiteral("qml"))
        return QStringLiteral("QML");
    if (ext == QStringLiteral("md") || ext == QStringLiteral("markdown"))
        return QStringLiteral("Markdown");
    if (ext == QStringLiteral("json"))
        return QStringLiteral("JSON");
    if (ext == QStringLiteral("xml") || ext == QStringLiteral("ui"))
        return QStringLiteral("XML");
    return ext.toUpper();
}
