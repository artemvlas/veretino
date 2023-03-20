#ifndef JSONDB_H
#define JSONDB_H

#include <QObject>
#include "QJsonDocument"
#include "QJsonObject"
#include "QJsonArray"
#include "datacontainer.h"

// This class is part of the Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
// Used to create json databases from Veretino DataContainer objects and vice versa.

class jsonDB : public QObject
{
    Q_OBJECT
public:
    explicit jsonDB(QObject *parent = nullptr);

    DataContainer* parseJson(const QString &pathToFile);
    void makeJson(DataContainer *data, const QString &about);

private:
    QJsonDocument readJsonFile(const QString &filePath);
    bool saveJsonFile(const QJsonDocument &document, const QString &filePath);
    QJsonArray loadJsonDB(const QString &filePath);

signals:
    void showMessage(const QString &text, const QString &title = "Info");
};

#endif // JSONDB_H
