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
    bool coloredDbItems = true;

    QByteArray geometryMainWindow;
    QByteArray headerStateFs;
    QByteArray headerStateDb;

    QString *lastFsPath = nullptr; // pointer to ui->treeView->curPathFileSystem

private:
    QCryptographicHash::Algorithm algorithm_ = QCryptographicHash::Sha256;
signals:
    void algorithmChanged();

}; // class Settings

#endif // SETTINGS_H
