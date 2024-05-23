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

    QString dbFileExtension() const;
    void addRecentFile(const QString &filePath);
    void clearRecentFiles();
    static QString dbFileExtension(bool isLong);

    void saveSettings();
    void loadSettings();

    // variables
    QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha256;
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

    QString lastFsPath;

}; // class Settings

#endif // SETTINGS_H
