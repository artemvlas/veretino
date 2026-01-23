/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/

#include "settings.h"
#include <QSettings>
#include "tools.h"

const QString Settings::s_key_algo = "algorithm";
const QString Settings::s_key_dbPrefix = "dbPrefix";
const QString Settings::s_key_restoreLastPath = "restoreLastPathOnStartup";
const QString Settings::s_key_addWorkDir = "addWorkDirToFilename";
const QString Settings::s_key_isLongExt = "isLongExtension";
const QString Settings::s_key_saveVerifDate = "saveVerifDate";
const QString Settings::s_key_dbFlagConst = "dbFlagConst";
const QString Settings::s_key_instantSaving = "instantSaving";
const QString Settings::s_key_considerDateModified = "considerDateModified";
const QString Settings::s_key_detectMoved = "detectMoved";
const QString Settings::s_key_allowPasteIntoDb = "allowPasteIntoDb";
const QString Settings::s_key_importSumsWhenItemAdding = "importSumsWhenItemAdding";

// history
const QString Settings::s_key_history_lastFsPath = "history/lastFsPath";
const QString Settings::s_key_history_recentDbFiles = "history/recentDbFiles";

// view
const QString Settings::s_key_view_geometry = "view/geometry";
const QString Settings::s_key_view_columnStateFs = "view/columnStateFs";
const QString Settings::s_key_view_columnStateDb = "view/columnStateDb";

// filter
const QString Settings::s_key_filter_mode = "filter/mode";
const QString Settings::s_key_filter_last_exts = "filter/last_exts";
const QString Settings::s_key_filter_ignore_db = "filter/ignore_db";
const QString Settings::s_key_filter_ignore_sha = "filter/ignore_sha";
const QString Settings::s_key_filter_remember_exts = "filter/remember_exts";
const QString Settings::s_key_filter_editable_exts = "filter/editable_exts";
const QString Settings::s_key_filter_ignore_unpermitted = "filter/ignore_unpermitted";
const QString Settings::s_key_filter_ignore_symlinks = "filter/ignore_symlinks";

Settings::Settings(QObject *parent)
    : QObject{parent}
{}

void Settings::setAlgorithm(QCryptographicHash::Algorithm algo)
{
    if (algorithm_ != algo) {
        algorithm_ = algo;
        emit algorithmChanged();
    }
}

QCryptographicHash::Algorithm Settings::algorithm() const
{
    return algorithm_;
}

QString Settings::dbFileExtension() const
{
    return dbFileExtension(isLongExtension);
}

QString Settings::dbFileExtension(bool isLong)
{
    return Lit::sl_db_exts.at(isLong ? 0 : 1); // "ver.json" : "ver";
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

    storedSettings.setValue(s_key_algo, algorithm_);
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
    storedSettings.setValue(s_key_view_geometry, geometryMainWindow);

    // TreeView header(columns) state
    storedSettings.setValue(s_key_view_columnStateFs, headerStateFs);
    storedSettings.setValue(s_key_view_columnStateDb, headerStateDb);
}

void Settings::loadSettings()
{
    QSettings storedSettings(QSettings::IniFormat, QSettings::UserScope, Lit::s_app_name, Lit::s_app_name);

    if (pLastFsPath) {
        *pLastFsPath = storedSettings.value(s_key_history_lastFsPath).toString();
    }

    Settings defaults;
    algorithm_ = static_cast<QCryptographicHash::Algorithm>(storedSettings.value(s_key_algo, defaults.algorithm()).toInt());
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
    geometryMainWindow = storedSettings.value(s_key_view_geometry).toByteArray();

    // TreeView header(columns) state
    headerStateFs = storedSettings.value(s_key_view_columnStateFs).toByteArray();
    headerStateDb = storedSettings.value(s_key_view_columnStateDb).toByteArray();
}
