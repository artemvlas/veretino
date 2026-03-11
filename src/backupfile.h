/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef BACKUPFILE_H
#define BACKUPFILE_H

#include "datacontainer.h"

class BackupFile final
{
public:
    explicit BackupFile(const DataContainer *data);
    explicit BackupFile(const MetaData *meta);
    explicit BackupFile(const QString *dbFile);

    explicit operator bool() const { return hasDbFilePath(); }

    QString backupFilePath() const;
    bool isBackupExists() const;
    bool isDbExists() const;

    bool makeBackup(bool forceOverwrite = false) const;
    bool restoreBackupFile() const;
    void removeBackupFile() const;

private:
    bool hasDbFilePath() const;

    const QString *m_dbFile = nullptr;
}; // class BackupFile

#endif // BACKUPFILE_H
