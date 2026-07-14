#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

enum class ProjectNodeKind {
    Project,
    Folder,
    File,
};

struct LineMetrics {
    int total = 0;
    int code = 0;
    int blank = 0;
    int comment = 0;
};

struct ProjectNode {
    ProjectNodeKind kind = ProjectNodeKind::File;
    QString name;
    QString path;
    QString language;
    qint64 sizeBytes = 0;
    LineMetrics lines;
    QDateTime createdAt;
    QDateTime modifiedAt;
    ProjectNode *parent = nullptr;
    std::vector<std::unique_ptr<ProjectNode>> children;

    int depth = 0;
    qint64 weight() const;
    bool isFolder() const;
    int fileCount() const;
    int folderCount() const;
    void aggregate();
};

class ProjectScanner {
public:
    struct Options {
        QStringList extensions;
        QStringList ignoredDirectoryNames;
    };

    static Options defaultOptions();
    static std::unique_ptr<ProjectNode> scan(const QString &rootPath,
                                             const Options &options = defaultOptions());
    static LineMetrics countLines(const QByteArray &contents, const QString &extension);

private:
    static void scanDirectory(ProjectNode &parent, const Options &options);
    static bool isSourceFile(const QString &fileName, const Options &options);
    static QString languageForExtension(const QString &extension);
};
