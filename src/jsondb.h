/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
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

    bool considerFileModDate = false;
    bool _cacheMissingChecksums = false;

private:   
    QJsonDocument readJsonFile(const QString &filePath);
    bool saveJsonFile(const QJsonDocument &document, const QString &filePath);
    QJsonArray loadJsonDB(const QString &filePath);
    QJsonObject dbHeader(const DataContainer *data, const QModelIndex &rootFolder);
    MetaData getMetaData(const QString &filePath, const QJsonObject &header, const QJsonObject &fileList);
    FileValues makeFileValues(const QString &filePath, const QString &basicDate);
    bool isPresentInWorkDir(const QString &workDir, const QJsonObject &fileList);
    QString findValueStr(const QJsonObject &object, const QString &approxKey, int sampleLength = 4);
    bool isCanceled() const;

    static const QString h_key_DateTime;
    static const QString h_key_Ignored;
    static const QString h_key_Included;
    static const QString h_key_Algo;
    static const QString h_key_WorkDir;
    static const QString h_key_Flags;

    static const QString h_key_Updated;
    static const QString h_key_Verified;

    static const QString a_key_Unreadable;

    QString jsonFilePath;
    const ProcState *proc_ = nullptr;

signals:
    void showMessage(const QString &text, const QString &title = "Info");
    void setStatusbarText(const QString &text = QString()); // text to statusbar
}; // class JsonDb

#endif // JSONDB_H
