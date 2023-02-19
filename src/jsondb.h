#ifndef JSONDB_H
#define JSONDB_H

#include <QObject>
#include "QJsonDocument"
#include "QJsonObject"
#include "QJsonArray"
#include "QFile"
#include "QDir"
#include "files.h"
#include "datacontainer.h"

class jsonDB : public QObject
{
    Q_OBJECT
public:
    explicit jsonDB(const QString &path = QString(), QObject *parent = nullptr);

    bool makeJsonDB(DataContainer *data);
    DataContainer* parseJson(const QString &pathToFile = QString());

    QString filePath;
    QString folderPath;

private:
    QJsonDocument readJsonFile(const QString &pathToFile = QString());
    bool saveJsonFile(const QJsonDocument &document, const QString &pathToFile = QString());
    QJsonArray loadJsonDB(const QString &pathToFile = QString());
    int shatypeByLen(const int &len);

signals:
    void status(const QString &text); //text to statusbar
    void showMessage(const QString &text, const QString &title = "Info");
};

#endif // JSONDB_H
