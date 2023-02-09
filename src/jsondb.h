#ifndef JSONDB_H
#define JSONDB_H

#include <QObject>
#include "QJsonDocument"
#include "QJsonObject"
#include "QJsonArray"
#include "QFile"
#include "QDir"
#include "files.h"

class jsonDB : public QObject
{
    Q_OBJECT
public:
    explicit jsonDB(const QString &path = QString(), QObject *parent = nullptr);

    bool makeJsonDB(const QMap<QString,QString> &dataMap, const QString &filename = QString());
    QMap<QString,QString> parseJson(const QString &pathToFile = QString());

    QStringList filteredExtensions;
    QString filePath;
    QString folderPath;
    int dbShaType; // 1 or 256 or 512: from json database header or by checksum lenght

private:
    QJsonDocument readJsonFile(const QString &pathToFile = QString());
    bool saveJsonFile(const QJsonDocument &document, const QString &pathToFile = QString());
    QJsonArray loadJsonDB(const QString &pathToFile = QString());
    int shatypeByLen(const int &len);
    //QString currentFolder;
    //QString currentFile;

signals:
    void status(const QString &text); //text to statusbar
    void showMessage(const QString &text, const QString &title = "Info");
};

#endif // JSONDB_H
