/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
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

    DataContainer* parseJson(const QString &filePath);
    QString makeJson(const DataContainer *data, const QModelIndex &rootFolder = QModelIndex());
    bool updateSuccessfulCheckDateTime(const QString &filePath);

public slots:
    void cancelProcess();

private:   
    QJsonDocument readJsonFile(const QString &filePath);
    bool saveJsonFile(const QJsonDocument &document, const QString &filePath);
    QJsonArray loadJsonDB(const QString &filePath);
    QJsonObject dbHeader(const DataContainer *data, const QModelIndex &rootFolder);

    const QString strHeaderIgnored = "Ignored";
    const QString strHeaderIncluded = "Included";
    const QString strHeaderAlgo = "Used algorithm";
    const QString strHeaderWorkDir = "Working folder";

    QString jsonFilePath;
    QElapsedTimer elapsedTimer;
    bool canceled = false;

signals:
    void showMessage(const QString &text, const QString &title = "Info");
    void setStatusbarText(const QString &text = QString()); // text to statusbar
};

#endif // JSONDB_H
