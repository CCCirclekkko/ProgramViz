#include "../src/projectmodel.h"
#include "../src/visualsettings.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include <QtTest/QtTest>

class ProjectModelTest final : public QObject {
    Q_OBJECT

private slots:
    void countsCodeAndComments();
    void scansCurrentProjectFixture();
    void aggregatesFolderTotals();
    void convertsHsiPaletteToRgb();
};

void ProjectModelTest::countsCodeAndComments()
{
    const QByteArray source(
        "#include <vector>\n"
        "\n"
        "// a comment\n"
        "int main() { /* inline */\n"
        "  return 0;\n"
        "}\n");
    const LineMetrics metrics = ProjectScanner::countLines(source, QStringLiteral("cpp"));
    QCOMPARE(metrics.total, 6);
    QCOMPARE(metrics.blank, 1);
    QCOMPARE(metrics.comment, 1);
    QCOMPARE(metrics.code, 4);
}

void ProjectModelTest::scansCurrentProjectFixture()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("src/nested"))));

    QFile cpp(directory.filePath(QStringLiteral("src/main.cpp")));
    QVERIFY(cpp.open(QIODevice::WriteOnly));
    cpp.write("int main() {\n  return 0;\n}\n");
    cpp.close();

    QFile ignored(directory.filePath(QStringLiteral("src/nested/readme.txt")));
    QVERIFY(ignored.open(QIODevice::WriteOnly));
    ignored.write("not a configured source file\n");
    ignored.close();

    QFile buildFile(directory.filePath(QStringLiteral("CMakeLists.txt")));
    QVERIFY(buildFile.open(QIODevice::WriteOnly));
    buildFile.write("cmake_minimum_required(VERSION 3.21)\n");
    buildFile.close();

    auto root = ProjectScanner::scan(directory.path());
    QVERIFY(root != nullptr);
    QCOMPARE(root->fileCount(), 2);
    QCOMPARE(root->lines.code, 4);
    QCOMPARE(root->children.size(), size_t(2));
    QCOMPARE(root->children.front()->name, QStringLiteral("src"));
    QCOMPARE(root->children.back()->name, QStringLiteral("CMakeLists.txt"));
}

void ProjectModelTest::aggregatesFolderTotals()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QFile file(directory.filePath(QStringLiteral("a.cpp")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("int a;\nint b;\n");
    file.close();

    auto root = ProjectScanner::scan(directory.path());
    QVERIFY(root != nullptr);
    QCOMPARE(root->lines.total, 2);
    QCOMPARE(root->lines.code, 2);
    QCOMPARE(root->weight(), qint64(2));
    QVERIFY(root->sizeBytes > 0);
}

void ProjectModelTest::convertsHsiPaletteToRgb()
{
    const QColor red = colorFromHsi(0, 100, 33);
    const QColor green = colorFromHsi(120, 100, 33);
    const QColor blue = colorFromHsi(240, 100, 33);
    QVERIFY(red.red() > red.green());
    QVERIFY(green.green() > green.red());
    QVERIFY(blue.blue() > blue.green());
    const QColor neutral = colorFromHsi(0, 0, 50);
    QCOMPARE(neutral.red(), 128);
    QCOMPARE(neutral.green(), 128);
    QCOMPARE(neutral.blue(), 128);
}

QTEST_MAIN(ProjectModelTest)
#include "projectmodel_test.moc"
