// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include <QObject>
#include <QCryptographicHash>
#include "treemodel.h"
#include "proxymodel.h"

class TreeModel;

struct MetaData {
    QCryptographicHash::Algorithm algorithm;
    QString workDir; // current working folder
    QString databaseFilePath;
    QString saveDateTime; // date and time the database was saved
    QString about; // contains a brief description of the item changes or status, if any
    FilterRule filter;
}; // struct MetaData

struct Numbers {
    int numChecksums = 0; // number of files with checksums
    qint64 totalSize = 0; // total size in bytes of all actual files for which there are checksums listed

    QHash<FileStatus, int> holder; // {enum FileStatus : int number of corresponding files}

    int numberOf(FileStatus status) const
    {
        return holder.contains(status) ? holder.value(status) : 0;
    }

    int available()
    {
        return numChecksums - numberOf(FileStatus::Missing);
    }
}; // struct Numbers

class DataContainer : public QObject
{
    Q_OBJECT
public:
    explicit DataContainer(QObject *parent = nullptr);

    ProxyModel* setProxyModel();
    QString databaseFileName() const;
    QString backupFilePath() const;
    bool isWorkDirRelative() const;
    bool isFilterApplied() const;

    enum ModelSelect {ModelSource, ModelProxy};
    //Q_ENUM(ModelSelect)

    TreeModel *model_ = new TreeModel(this);  // main data
    ProxyModel *proxyModel_ = new ProxyModel(model_, this);
    MetaData metaData;
    Numbers numbers;
}; // class DataContainer

using ModelSelect = DataContainer::ModelSelect;

#endif // DATACONTAINER_H
