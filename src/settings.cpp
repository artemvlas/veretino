/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/

#include "settings.h"
#include <QSettings>
#include <QDebug>
#include "tools.h"

const QString Settings::_str_veretino = "veretino";

const QString Settings::s_key_algo = "algorithm";
const QString Settings::s_key_dbPrefix = "dbPrefix";
const QString Settings::s_key_restoreLastPath = "restoreLastPathOnStartup";
const QString Settings::s_key_addWorkDir = "addWorkDirToFilename";
const QString Settings::s_key_isLongExt = "isLongExtension";
const QString Settings::s_key_saveVerifDate = "saveVerifDate";
const QString Settings::s_key_dbFlagConst = "dbFlagConst";
const QString Settings::s_key_instantSaving = "instantSaving";
const QString Settings::s_key_excludeUnPerm = "excludeUnPerm";
const QString Settings::s_key_filter_ignoreDbFiles = "filter/ignoreDbFiles";
const QString Settings::s_key_filter_ignoreShaFiles = "filter/ignoreShaFiles";
const QString Settings::s_key_filter_Mode = "filter/filterMode";
const QString Settings::s_key_filter_ExtList = "filter/filterExtList";
const QString Settings::s_key_history_lastFsPath = "history/lastFsPath";
const QString Settings::s_key_history_recentDbFiles = "history/recentDbFiles";
const QString Settings::s_key_view_geometry = "view/geometry";
const QString Settings::s_key_view_columnStateFs = "view/columnStateFs";
const QString Settings::s_key_view_columnStateDb = "view/columnStateDb";

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
    return Lit::sl_db_exts.at(isLong ? 1 : 0); // "ver.json" : "ver";
}

void Settings::addRecentFile(const QString &filePath)
{
    int ind = recentFiles.indexOf(filePath);
    if (ind == -1) {
        recentFiles.prepend(filePath); // add to the top of the list
    }
    else if (ind > 0) {
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
    QSettings storedSettings(QSettings::IniFormat, QSettings::UserScope, _str_veretino, _str_veretino);
    // qDebug() << "Save settings:" << storedSettings.fileName() <<  storedSettings.format();

    if (lastFsPath) {
        storedSettings.setValue(s_key_history_lastFsPath, restoreLastPathOnStartup ? *lastFsPath : QString());
    }

    storedSettings.setValue(s_key_algo, algorithm_);
    storedSettings.setValue(s_key_dbPrefix, dbPrefix);
    storedSettings.setValue(s_key_restoreLastPath, restoreLastPathOnStartup);
    storedSettings.setValue(s_key_addWorkDir, addWorkDirToFilename);
    storedSettings.setValue(s_key_isLongExt, isLongExtension);
    storedSettings.setValue(s_key_saveVerifDate, saveVerificationDateTime);
    storedSettings.setValue(s_key_dbFlagConst, dbFlagConst);
    storedSettings.setValue(s_key_instantSaving, instantSaving);
    storedSettings.setValue(s_key_excludeUnPerm, excludeUnpermitted);

    // FilterRule
    storedSettings.setValue(s_key_filter_ignoreDbFiles, filter.ignoreDbFiles);
    storedSettings.setValue(s_key_filter_ignoreShaFiles, filter.ignoreShaFiles);
    storedSettings.setValue(s_key_filter_Mode, filter.mode());
    storedSettings.setValue(s_key_filter_ExtList, filter.extensionList());

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
    QSettings storedSettings(QSettings::IniFormat, QSettings::UserScope, _str_veretino, _str_veretino);
    // qDebug() << "Load settings:" << storedSettings.fileName() << storedSettings.format();

    if (lastFsPath) {
        *lastFsPath = storedSettings.value(s_key_history_lastFsPath).toString();
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
    excludeUnpermitted = storedSettings.value(s_key_excludeUnPerm, defaults.excludeUnpermitted).toBool();

    // FilterRule
    filter.setFilter(static_cast<FilterRule::FilterMode>(storedSettings.value(s_key_filter_Mode, FilterRule::NotSet).toInt()),
                                                            storedSettings.value(s_key_filter_ExtList).toStringList());
    filter.ignoreDbFiles = storedSettings.value(s_key_filter_ignoreDbFiles, defaults.filter.ignoreDbFiles).toBool();
    filter.ignoreShaFiles = storedSettings.value(s_key_filter_ignoreShaFiles, defaults.filter.ignoreShaFiles).toBool();

    // recent files
    recentFiles = storedSettings.value(s_key_history_recentDbFiles).toStringList();

    // geometry
    geometryMainWindow = storedSettings.value(s_key_view_geometry).toByteArray();

    // TreeView header(columns) state
    headerStateFs = storedSettings.value(s_key_view_columnStateFs).toByteArray();
    headerStateDb = storedSettings.value(s_key_view_columnStateDb).toByteArray();
}
