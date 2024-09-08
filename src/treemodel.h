/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include "treeitem.h"
#include "files.h"
#include "iconprovider.h"

class TreeItem;

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit TreeModel(QObject *parent = nullptr);
    ~TreeModel();

    enum ItemDataRoles { RawDataRole = 1000 };
    enum Column { ColumnName, ColumnSize, ColumnStatus, ColumnChecksum, ColumnReChecksum };
    Q_ENUM(Column)

    QVariant data(const QModelIndex &curIndex, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &curIndex) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &curIndex) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool setData(const QModelIndex &curIndex, const QVariant &value,
                        int role = Qt::EditRole) override;
    bool setRowData(const QModelIndex &curIndex, Column column,
                    const QVariant &value = QVariant());

    bool isEmpty() const;
    void add_file(const QString &filePath, const FileValues &values); // new func., with cache
    bool add_file_unforced(const QString &filePath, const FileValues &values); // checks for presence first; much slower for large lists
    void populate(const FileList &filesData);

    static QString getPath(const QModelIndex &curIndex, const QModelIndex &root = QModelIndex()); // build path by current index data
    static QModelIndex getIndex(const QString &path, const QAbstractItemModel *model); // find index of specified 'path'
    static bool isFileRow(const QModelIndex &curIndex); // whether the row of curIndex corresponds to a file(true) or (folder(false) || invalid(false))
    static bool isFolderRow(const QModelIndex &curIndex); // same^, but folder(true); (file(false) || invalid(false))
    static bool hasChecksum(const QModelIndex &fileIndex);
    static bool hasReChecksum(const QModelIndex &fileIndex);
    static bool hasStatus(const FileStatuses flag, const QModelIndex &fileIndex);
    static bool contains(const FileStatuses flag, const QModelIndex &folderIndex);

    static QString itemName(const QModelIndex &curIndex);
    static qint64 itemFileSize(const QModelIndex &fileIndex);
    static FileStatus itemFileStatus(const QModelIndex &fileIndex);
    static QString itemFileChecksum(const QModelIndex &fileIndex);
    static QString itemFileReChecksum(const QModelIndex &fileIndex);

public slots:
    void clearCacheFolderItems();

private:
    TreeItem *getItem(const QModelIndex &curIndex) const;
    TreeItem *add_folder(const QString &path);

    TreeItem *rootItem;
    IconProvider icons_;
    QHash<QString, TreeItem*> cacheFolderItems_;
}; // class TreeModel

using Column = TreeModel::Column;

#endif // TREEMODEL_H
