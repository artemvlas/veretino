// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
// Used to create json databases from Veretino DataContainer objects and vice versa
#ifndef JSONDB_H
#define JSONDB_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "datacontainer.h"
#include <QElapsedTimer>

class JsonDb : public QObject
{
    Q_OBJECT
public:
    explicit JsonDb(QObject *parent = nullptr);
    explicit JsonDb(const QString &filePath, QObject *parent = nullptr);

    enum Result {Saved, SavedToDesktop, NotSaved, Canceled};
    Q_ENUM(Result)

    DataContainer* parseJson(const QString &filePath);
    Result makeJson(const DataContainer* data);

public slots:
    void cancelProcess();

private:   
    QJsonDocument readJsonFile(const QString &filePath);
    bool saveJsonFile(const QJsonDocument &document, const QString &filePath);
    QJsonArray loadJsonDB(const QString &filePath);

    QString strHeaderIgnored = "Ignored";
    QString strHeaderIncluded = "Included";
    QString strHeaderAlgo = "Used algorithm";
    QString strHeaderWorkDir = "Working folder";

    QString jsonFilePath;
    QElapsedTimer elapsedTimer;
    bool canceled = false;

signals:
    void showMessage(const QString &text, const QString &title = "Info");
    void setStatusbarText(const QString &text = QString()); // text to statusbar
};

#endif // JSONDB_H
