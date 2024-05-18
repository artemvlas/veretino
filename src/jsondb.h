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
#include "procstate.h"

class JsonDb : public QObject
{
    Q_OBJECT
public:
    explicit JsonDb(QObject *parent = nullptr);
    explicit JsonDb(const QString &filePath, QObject *parent = nullptr);
    void setProcState(const ProcState *procState);

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
    MetaData getMetaData(const QString &filePath, const QJsonObject &header, const QJsonObject &fileList);
    bool isPresentInWorkDir(const QString &workDir, const QJsonObject &fileList);
    QString findValueStr(const QJsonObject &object, const QString &approxKey, int sampleLength = 4);

    const QString strHeaderIgnored = "Ignored";
    const QString strHeaderIncluded = "Included";
    const QString strHeaderAlgo = "Used algorithm";
    const QString strHeaderWorkDir = "WorkDir";

    QString jsonFilePath;
    const ProcState *proc_ = nullptr;
    bool canceled = false;

signals:
    void showMessage(const QString &text, const QString &title = "Info");
    void setStatusbarText(const QString &text = QString()); // text to statusbar
};

#endif // JSONDB_H
