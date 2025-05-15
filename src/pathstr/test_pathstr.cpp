#include <QtTest/QTest>

#include "pathstr.h"

class test_pathstr : public QObject
{
    Q_OBJECT

public:
    test_pathstr();
    ~test_pathstr();

private slots:
    void test_basicName();
    void test_parentFolder();
    void test_relativePath();
    void test_joinPath();
    void test_composeFilePath();
    void test_root();
    void test_suffix();
    void test_setSuffix();
    void test_suffixSize();
    void test_isRoot();
    void test_hasExtension();
    void test_isSeparator();
    void test_endsWithSep();
};

test_pathstr::test_pathstr() {}

test_pathstr::~test_pathstr() {}

void test_pathstr::test_basicName()
{
    QCOMPARE(pathstr::basicName("/folder/file.txt"), "file.txt");
    QCOMPARE(pathstr::basicName("/folder/folder2/"), "folder2");
}

void test_pathstr::test_parentFolder()
{
    QCOMPARE(pathstr::parentFolder("/folder/file_or_folder2"), "/folder");
    QCOMPARE(pathstr::parentFolder("/folder/file_or_folder2/"), "/folder");
}

void test_pathstr::test_relativePath()
{
    QCOMPARE(pathstr::relativePath("/folder/rootFolder", "/folder/rootFolder/folder2/file"), "folder2/file");
    QCOMPARE(pathstr::relativePath("/folder/rootFolder/", "/folder/rootFolder/folder2/file"), "folder2/file");
}

void test_pathstr::test_joinPath()
{
    QCOMPARE(pathstr::joinPath("/home/folder", "folder2/file"), "/home/folder/folder2/file");
    QCOMPARE(pathstr::joinPath("/home/folder/", "folder2/file"), "/home/folder/folder2/file");
    QCOMPARE(pathstr::joinPath("/home/folder", "/folder2/file"), "/home/folder/folder2/file");
}

void test_pathstr::test_composeFilePath()
{
    QCOMPARE(pathstr::composeFilePath("/home/folder", "filename", "cpp"), "/home/folder/filename.cpp");
}

void test_pathstr::test_root()
{
    QCOMPARE(pathstr::root("C:/folder"), "C:/");
    QCOMPARE(pathstr::root("d:\\"), "d:\\");
    QCOMPARE(pathstr::root("/home"), "/");
    QCOMPARE(pathstr::root("/"), "/");
}

void test_pathstr::test_suffix()
{
    QCOMPARE(pathstr::suffix("file.txt"), "txt");
    QCOMPARE(pathstr::suffix("file.ver.json"), "json");
}

void test_pathstr::test_setSuffix()
{
    QCOMPARE(pathstr::setSuffix("file.txt", "cpp"), "file.cpp");
    QCOMPARE(pathstr::setSuffix("file.ver.json", "json"), "file.ver.json");
}

void test_pathstr::test_suffixSize()
{
    QCOMPARE(pathstr::suffixSize("file.txt"), 3);
    QCOMPARE(pathstr::suffixSize("file.ver.json"), 4);
}

void test_pathstr::test_isRoot()
{
    QCOMPARE(pathstr::isRoot("/"), true);
    QCOMPARE(pathstr::isRoot("D:\\"), true);
    QCOMPARE(pathstr::isRoot("c:/"), true);
    QCOMPARE(pathstr::isRoot("E:/folder"), false);
    QCOMPARE(pathstr::isRoot("/home"), false);
}

void test_pathstr::test_hasExtension()
{
    QCOMPARE(pathstr::hasExtension("file.cpp", "cpp"), true);
    QCOMPARE(pathstr::hasExtension("file.cpp", "ver"), false);
    QCOMPARE(pathstr::hasExtension("file.cpp", { "txt", "h", "cpp" }), true);
    QCOMPARE(pathstr::hasExtension("file.cpp", { "jpg", "h", "pdf" }), false);
}

void test_pathstr::test_isSeparator()
{
    QVERIFY(pathstr::isSeparator('/'));
    QVERIFY(pathstr::isSeparator('\\'));
}

void test_pathstr::test_endsWithSep()
{
    QVERIFY(pathstr::endsWithSep("/folder/"));
    QVERIFY(pathstr::endsWithSep("C:\\folder\\"));
}

QTEST_APPLESS_MAIN(test_pathstr)

#include "test_pathstr.moc"
