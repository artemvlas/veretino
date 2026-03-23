/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/

#include "settings.h"
#include <QSettings>
#include "tools.h"
#include "dbfileextension.h"

const QString Settings::s_key_algo = QStringLiteral(u"algorithm");
const QString Settings::s_key_dbPrefix = QStringLiteral(u"dbPrefix");
const QString Settings::s_key_restoreLastPath = QStringLiteral(u"restoreLastPathOnStartup");
const QString Settings::s_key_addWorkDir = QStringLiteral(u"addWorkDirToFilename");
const QString Settings::s_key_isLongExt = QStringLiteral(u"isLongExtension");
const QString Settings::s_key_saveVerifDate = QStringLiteral(u"saveVerifDate");
const QString Settings::s_key_dbFlagConst = QStringLiteral(u"dbFlagConst");
const QString Settings::s_key_instantSaving = QStringLiteral(u"instantSaving");
const QString Settings::s_key_considerDateModified = QStringLiteral(u"considerDateModified");
const QString Settings::s_key_detectMoved = QStringLiteral(u"detectMoved");
const QString Settings::s_key_allowPasteIntoDb = QStringLiteral(u"allowPasteIntoDb");
const QString Settings::s_key_importSumsWhenItemAdding = QStringLiteral(u"importSumsWhenItemAdding");

// history
const QString Settings::s_key_history_lastFsPath = QStringLiteral(u"history/lastFsPath");
const QString Settings::s_key_history_recentDbFiles = QStringLiteral(u"history/recentDbFiles");

// view
const QString Settings::s_key_view_geometry = QStringLiteral(u"view/geometry");
const QString Settings::s_key_view_columnStateFs = QStringLiteral(u"view/columnStateFs");
const QString Settings::s_key_view_columnStateDb = QStringLiteral(u"view/columnStateDb");

// filter
const QString Settings::s_key_filter_mode = QStringLiteral(u"filter/mode");
const QString Settings::s_key_filter_last_exts = QStringLiteral(u"filter/last_exts");
const QString Settings::s_key_filter_ignore_db = QStringLiteral(u"filter/ignore_db");
const QString Settings::s_key_filter_ignore_sha = QStringLiteral(u"filter/ignore_sha");
const QString Settings::s_key_filter_remember_exts = QStringLiteral(u"filter/remember_exts");
const QString Settings::s_key_filter_editable_exts = QStringLiteral(u"filter/editable_exts");
const QString Settings::s_key_filter_ignore_unpermitted = QStringLiteral(u"filter/ignore_unpermitted");
const QString Settings::s_key_filter_ignore_symlinks = QStringLiteral(u"filter/ignore_symlinks");

Settings::Settings(QObject *parent)
    : QObject{parent}
{}

void Settings::setAlgorithm(QCryptographicHash::Algorithm algo)
{
    if (m_algorithm != algo) {
        m_algorithm = algo;
        emit algorithmChanged();
    }
}

QCryptographicHash::Algorithm Settings::algorithm() const
{
    return m_algorithm;
}

QString Settings::dbFileExtension() const
{
    return dbFileExtension(isLongExtension);
}

QString Settings::dbFileExtension(bool isLong)
{
    // "ver.json" || "ver"
    return DbFileExtension::extension(isLong);
}

void Settings::addRecentFile(const QString &filePath)
{
    int ind = recentFiles.indexOf(filePath);
    if (ind == -1) {
        recentFiles.prepend(filePath); // add to the top of the list
    } else if (ind > 0) {
        recentFiles.move(ind, 0); // move the recent file to the top
    }

    if (recentFiles.size() > 15) {
        recentFiles.removeLast();
    }
}

void Settings::clearRecentFiles()
{
    recentFiles.clear();
}

void Settings::saveSettings()
{
    QSettings storedSettings(QSettings::IniFormat, QSettings::UserScope, Lit::s_app_name, Lit::s_app_name);
    // qDebug() << "Save settings:" << storedSettings.fileName() <<  storedSettings.format();

    if (pLastFsPath) {
        storedSettings.setValue(s_key_history_lastFsPath, restoreLastPathOnStartup ? *pLastFsPath : QString());
    }

    storedSettings.setValue(s_key_algo, m_algorithm);
    storedSettings.setValue(s_key_dbPrefix, dbPrefix);
    storedSettings.setValue(s_key_restoreLastPath, restoreLastPathOnStartup);
    storedSettings.setValue(s_key_addWorkDir, addWorkDirToFilename);
    storedSettings.setValue(s_key_isLongExt, isLongExtension);
    storedSettings.setValue(s_key_saveVerifDate, saveVerificationDateTime);
    storedSettings.setValue(s_key_dbFlagConst, dbFlagConst);
    storedSettings.setValue(s_key_instantSaving, instantSaving);
    storedSettings.setValue(s_key_considerDateModified, considerDateModified);
    storedSettings.setValue(s_key_detectMoved, detectMoved);
    storedSettings.setValue(s_key_allowPasteIntoDb, allowPasteIntoDb);
    storedSettings.setValue(s_key_importSumsWhenItemAdding, m_importSumsWhenItemAdding);

    // filter
    storedSettings.setValue(s_key_filter_mode, filter_mode);
    storedSettings.setValue(s_key_filter_last_exts, filter_last_exts);
    storedSettings.setValue(s_key_filter_editable_exts, filter_editable_exts);
    storedSettings.setValue(s_key_filter_remember_exts, filter_remember_exts);
    storedSettings.setValue(s_key_filter_ignore_db, filter_ignore_db);
    storedSettings.setValue(s_key_filter_ignore_sha, filter_ignore_sha);
    storedSettings.setValue(s_key_filter_ignore_unpermitted, filter_ignore_unpermitted);
    storedSettings.setValue(s_key_filter_ignore_symlinks, filter_ignore_symlinks);

    // recent files
    storedSettings.setValue(s_key_history_recentDbFiles, recentFiles);

    // geometry
    storedSettings.setValue(s_key_view_geometry, m_geometryMainWindow);

    // TreeView header(columns) state
    storedSettings.setValue(s_key_view_columnStateFs, m_headerStateFs);
    storedSettings.setValue(s_key_view_columnStateDb, m_headerStateDb);
}

void Settings::loadSettings()
{
    QSettings storedSettings(QSettings::IniFormat, QSettings::UserScope, Lit::s_app_name, Lit::s_app_name);

    if (pLastFsPath) {
        *pLastFsPath = storedSettings.value(s_key_history_lastFsPath).toString();
    }

    Settings defaults;
    m_algorithm = static_cast<QCryptographicHash::Algorithm>(storedSettings.value(s_key_algo, defaults.algorithm()).toInt());
    dbPrefix = storedSettings.value(s_key_dbPrefix, defaults.dbPrefix).toString();
    restoreLastPathOnStartup = storedSettings.value(s_key_restoreLastPath, defaults.restoreLastPathOnStartup).toBool();
    addWorkDirToFilename = storedSettings.value(s_key_addWorkDir, defaults.addWorkDirToFilename).toBool();
    isLongExtension = storedSettings.value(s_key_isLongExt, defaults.isLongExtension).toBool();
    saveVerificationDateTime = storedSettings.value(s_key_saveVerifDate, defaults.saveVerificationDateTime).toBool();
    dbFlagConst = storedSettings.value(s_key_dbFlagConst, defaults.dbFlagConst).toBool();
    instantSaving = storedSettings.value(s_key_instantSaving, defaults.instantSaving).toBool();
    considerDateModified = storedSettings.value(s_key_considerDateModified, defaults.considerDateModified).toBool();
    detectMoved = storedSettings.value(s_key_detectMoved, defaults.detectMoved).toBool();
    allowPasteIntoDb = storedSettings.value(s_key_allowPasteIntoDb, defaults.allowPasteIntoDb).toBool();
    m_importSumsWhenItemAdding = storedSettings.value(s_key_importSumsWhenItemAdding, defaults.m_importSumsWhenItemAdding).toBool();

    // filter
    filter_mode = static_cast<FilterMode>(storedSettings.value(s_key_filter_mode, FilterMode::NotSet).toInt());
    filter_last_exts = storedSettings.value(s_key_filter_last_exts).toStringList();
    filter_editable_exts = storedSettings.value(s_key_filter_editable_exts, defaults.filter_editable_exts).toBool();
    filter_remember_exts = storedSettings.value(s_key_filter_remember_exts, defaults.filter_remember_exts).toBool();
    filter_ignore_db = storedSettings.value(s_key_filter_ignore_db, defaults.filter_ignore_db).toBool();
    filter_ignore_sha = storedSettings.value(s_key_filter_ignore_sha, defaults.filter_ignore_db).toBool();
    filter_ignore_unpermitted = storedSettings.value(s_key_filter_ignore_unpermitted, defaults.filter_ignore_unpermitted).toBool();
    filter_ignore_symlinks = storedSettings.value(s_key_filter_ignore_symlinks, defaults.filter_ignore_symlinks).toBool();

    // recent files
    recentFiles = storedSettings.value(s_key_history_recentDbFiles).toStringList();

    // geometry
    m_geometryMainWindow = storedSettings.value(s_key_view_geometry).toByteArray();

    // TreeView header(columns) state
    m_headerStateFs = storedSettings.value(s_key_view_columnStateFs).toByteArray();
    m_headerStateDb = storedSettings.value(s_key_view_columnStateDb).toByteArray();
}
