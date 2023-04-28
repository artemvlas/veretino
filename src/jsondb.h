#ifndef JSONDB_H
#define JSONDB_H

#include <QObject>
#include "QJsonDocument"
#include "QJsonObject"
#include "QJsonArray"
#include "datacontainer.h"

// This class is part of the Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
// Used to create json databases from Veretino DataContainer objects and vice versa.

class JsonDb : public QObject
{
    Q_OBJECT
public:
    explicit JsonDb(QObject *parent = nullptr);
    explicit JsonDb(const QString &filePath, QObject *parent = nullptr);

    DataContainer parseJson(const QString &filePath);
    void makeJson(const DataContainer &data);

private:
    QString jsonFilePath;
    QJsonDocument readJsonFile(const QString &filePath);
    bool saveJsonFile(const QJsonDocument &document, const QString &filePath);
    QJsonArray loadJsonDB(const QString &filePath);

signals:
    void showMessage(const QString &text, const QString &title = "Info");
    void status(const QString &text = QString()); // text to statusbar
};

#endif // JSONDB_H
