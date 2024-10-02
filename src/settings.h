/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QCryptographicHash>
#include "filterrule.h"

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QObject *parent = nullptr);

    void setAlgorithm(QCryptographicHash::Algorithm algo);
    QCryptographicHash::Algorithm algorithm() const;
    QString dbFileExtension() const;
    void addRecentFile(const QString &filePath);
    void clearRecentFiles();
    static QString dbFileExtension(bool isLong);

    void saveSettings();
    void loadSettings();

    // variables
    FilterRule filter;
    QStringList recentFiles;
    QString dbPrefix = "checksums";
    bool restoreLastPathOnStartup = true;
    bool addWorkDirToFilename = true;
    bool isLongExtension = true;
    bool saveVerificationDateTime = false;
    bool instantSaving = false;
    bool excludeUnpermitted = true;
    bool dbFlagConst = false;
    bool considerDateModified = false;

    QByteArray geometryMainWindow;
    QByteArray headerStateFs;
    QByteArray headerStateDb;

    QString *lastFsPath = nullptr; // pointer to ui->treeView->curPathFileSystem

private:
    QCryptographicHash::Algorithm algorithm_ = QCryptographicHash::Sha256;

    static const QString s_key_algo;
    static const QString s_key_dbPrefix;
    static const QString s_key_restoreLastPath;
    static const QString s_key_addWorkDir;
    static const QString s_key_isLongExt;
    static const QString s_key_saveVerifDate;
    static const QString s_key_dbFlagConst;
    static const QString s_key_instantSaving;
    static const QString s_key_excludeUnPerm;
    static const QString s_key_considerDateModified;
    static const QString s_key_filter_ignoreDbFiles;
    static const QString s_key_filter_ignoreShaFiles;
    static const QString s_key_filter_Mode;
    static const QString s_key_filter_ExtList;
    static const QString s_key_history_lastFsPath;
    static const QString s_key_history_recentDbFiles;
    static const QString s_key_view_geometry;
    static const QString s_key_view_columnStateFs;
    static const QString s_key_view_columnStateDb;

signals:
    void algorithmChanged();
}; // class Settings

#endif // SETTINGS_H
