/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/

#include "settings.h"
#include <QSettings>
#include <QDebug>

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
    return isLong ? ".ver.json" : ".ver";
}

void Settings::addRecentFile(const QString &filePath)
{
    int ind = recentFiles.indexOf(filePath);
    if (ind == -1)
        recentFiles.prepend(filePath); // add to the top of the list
    else if (ind > 0)
        recentFiles.move(ind, 0); // move the recent file to the top

    if (recentFiles.size() > 15)
        recentFiles.removeLast();
}

void Settings::clearRecentFiles()
{
    recentFiles.clear();
}

void Settings::saveSettings()
{
    QSettings storedSettings(QSettings::IniFormat, QSettings::UserScope, "veretino", "veretino");
    qDebug() << "Save settings:" << storedSettings.fileName() <<  storedSettings.format();

    if (lastFsPath)
        storedSettings.setValue("history/lastFsPath", restoreLastPathOnStartup ? *lastFsPath : QString());

    storedSettings.setValue("algorithm", algorithm_);
    storedSettings.setValue("dbPrefix", dbPrefix);
    storedSettings.setValue("restoreLastPathOnStartup", restoreLastPathOnStartup);
    storedSettings.setValue("addWorkDirToFilename", addWorkDirToFilename);
    storedSettings.setValue("isLongExtension", isLongExtension);
    storedSettings.setValue("saveVerificationDateTime", saveVerificationDateTime);
    storedSettings.setValue("coloredDbItems", coloredDbItems);
    storedSettings.setValue("instantSaving", instantSaving);
    storedSettings.setValue("showUpdNumInMenu", showUpdNumInMenu);

    // FilterRule
    storedSettings.setValue("filter/ignoreDbFiles", filter.ignoreDbFiles);
    storedSettings.setValue("filter/ignoreShaFiles", filter.ignoreShaFiles);
    storedSettings.setValue("filter/filterMode", filter.mode_);
    storedSettings.setValue("filter/filterExtensionsList", filter.extensionsList);

    // recent files
    storedSettings.setValue("history/recentDbFiles", recentFiles);

    // geometry
    storedSettings.setValue("view/geometry", geometryMainWindow);

    // TreeView header(columns) state
    storedSettings.setValue("view/columnStateFs", headerStateFs);
    storedSettings.setValue("view/columnStateDb", headerStateDb);
}

void Settings::loadSettings()
{
    QSettings storedSettings(QSettings::IniFormat, QSettings::UserScope, "veretino", "veretino");
    qDebug() << "Load settings:" << storedSettings.fileName() << storedSettings.format();

    if (lastFsPath)
        *lastFsPath = storedSettings.value("history/lastFsPath").toString();

    Settings defaults;
    algorithm_ = static_cast<QCryptographicHash::Algorithm>(storedSettings.value("algorithm", defaults.algorithm()).toInt());
    dbPrefix = storedSettings.value("dbPrefix", defaults.dbPrefix).toString();
    restoreLastPathOnStartup = storedSettings.value("restoreLastPathOnStartup", defaults.restoreLastPathOnStartup).toBool();
    addWorkDirToFilename = storedSettings.value("addWorkDirToFilename", defaults.addWorkDirToFilename).toBool();
    isLongExtension = storedSettings.value("isLongExtension", defaults.isLongExtension).toBool();
    saveVerificationDateTime = storedSettings.value("saveVerificationDateTime", defaults.saveVerificationDateTime).toBool();
    coloredDbItems = storedSettings.value("coloredDbItems", defaults.coloredDbItems).toBool();
    instantSaving = storedSettings.value("instantSaving", defaults.instantSaving).toBool();
    showUpdNumInMenu = storedSettings.value("showUpdNumInMenu", defaults.showUpdNumInMenu).toBool();

    // FilterRule
    filter.setFilter(static_cast<FilterRule::FilterMode>(storedSettings.value("filter/filterMode", FilterRule::NotSet).toInt()),
                                storedSettings.value("filter/filterExtensionsList").toStringList());
    filter.ignoreDbFiles = storedSettings.value("filter/ignoreDbFiles", defaults.filter.ignoreDbFiles).toBool();
    filter.ignoreShaFiles = storedSettings.value("filter/ignoreShaFiles", defaults.filter.ignoreShaFiles).toBool();

    // recent files
    recentFiles = storedSettings.value("history/recentDbFiles").toStringList();

    // geometry
    geometryMainWindow = storedSettings.value("view/geometry").toByteArray();

    // TreeView header(columns) state
    headerStateFs = storedSettings.value("view/columnStateFs").toByteArray();
    headerStateDb = storedSettings.value("view/columnStateDb").toByteArray();
}
