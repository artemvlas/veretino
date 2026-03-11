#include "backupfile.h"
#include "pathstr.h"

BackupFile::BackupFile(const DataContainer *data)
    : m_dbFile(data ? &data->m_metadata.dbFilePath : nullptr) {}

BackupFile::BackupFile(const MetaData *meta)
    : m_dbFile(meta ? &meta->dbFilePath : nullptr) {}

BackupFile::BackupFile(const QString *dbFile)
    : m_dbFile(dbFile) {}

QString BackupFile::backupFilePath() const
{
    if (!hasDbFilePath())
        return {};

    return pathstr::prependFileName(*m_dbFile,
                                    QStringLiteral(u".tmp-backup_"));
}

bool BackupFile::isBackupExists() const
{
    return QFileInfo::exists(backupFilePath());
}

bool BackupFile::isDbExists() const
{
    return hasDbFilePath() && QFileInfo::exists(*m_dbFile);
}

bool BackupFile::makeBackup(bool forceOverwrite) const
{
    if (!isDbExists())
        return false;

    if (forceOverwrite)
        removeBackupFile();

    return QFile::copy(*m_dbFile, backupFilePath());
}

bool BackupFile::restoreBackupFile() const
{
    if (isBackupExists()) {
        if (isDbExists()) {
            if (!QFile::remove(*m_dbFile))
                return false;
        }
        return QFile::rename(backupFilePath(), *m_dbFile);
    }
    return false;
}

void BackupFile::removeBackupFile() const
{
    if (isBackupExists())
        QFile::remove(backupFilePath());
}

bool BackupFile::hasDbFilePath() const
{
    if (!m_dbFile) {
        qWarning() << Q_FUNC_INFO << "DB file is not set";
        return false;
    }

    return !(*m_dbFile).isEmpty();
}
