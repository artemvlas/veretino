/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "widgetfiletypes.h"

WidgetFileTypes::WidgetFileTypes(QWidget *parent)
    : QTreeWidget(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    icons_.setTheme(palette());
}

void WidgetFileTypes::setItems(const FileTypeList &extList)
{
    FileTypeList::const_iterator it;
    for (it = extList.constBegin(); it != extList.constEnd(); ++it) {
        QIcon _icon;
        const QString _ext = it.key();
        const NumSize _nums = it.value();

        if (_ext == Files::strVeretinoDb)
            _icon = icons_.icon(Icons::Database);
        else if (_ext == Files::strShaFiles)
            _icon = icons_.icon(Icons::HashFile);
        else if (_ext == Files::strNoPerm)
            _icon = icons_.icon(FileStatus::UnPermitted);
        else
            _icon = icons_.type_icon(_ext);

        ItemFileType *_item = new ItemFileType(this);
        _item->setData(ItemFileType::ColumnType, Qt::DisplayRole, _ext);
        _item->setData(ItemFileType::ColumnFilesNumber, Qt::DisplayRole, _nums._num);
        _item->setData(ItemFileType::ColumnTotalSize, Qt::DisplayRole, format::dataSizeReadable(_nums._size));
        _item->setData(ItemFileType::ColumnTotalSize, Qt::UserRole, _nums._size);
        _item->setIcon(ItemFileType::ColumnType, _icon);
        items_.append(_item);
    }
}

void WidgetFileTypes::setCheckboxesVisible(bool visible)
{
    static const QSet<QString> _s_excl { Files::strNoType, Files::strVeretinoDb, Files::strShaFiles, Files::strNoPerm, Files::strSymLink };

    this->blockSignals(true); // to avoid multiple calls &QTreeWidget::itemChanged --> ::updateFilterDisplay

    for (ItemFileType *_item : std::as_const(items_)) {
        _item->setCheckBoxVisible(visible
                                  && !_s_excl.contains(_item->extension()));
    }

    this->blockSignals(false);
}

QList<ItemFileType*> WidgetFileTypes::items(CheckState state) const
{
    QList<ItemFileType*> _res;

    for (ItemFileType *_item : std::as_const(items_)) {
        if (isPassed(state, _item))
            _res.append(_item);
    }

    return _res;
}

QStringList WidgetFileTypes::checkedExtensions() const
{
    QStringList _exts;
    const QList<ItemFileType*> _checked_items = items(Checked);

    for (const ItemFileType *_item : _checked_items) {
        _exts.append(_item->extension());
    }

    return _exts;
}

bool WidgetFileTypes::isPassedChecked(const ItemFileType *item) const
{
    // allow only visible_checked
    return (!item->isHidden() && item->isChecked());
}

bool WidgetFileTypes::isPassedUnChecked(const ItemFileType *item) const
{
    static const QSet<QString> _s_unfilt { Files::strVeretinoDb, Files::strShaFiles, Files::strNoPerm, Files::strSymLink };

    // allow all except visible_checked and Db-Sha
    return (!item->isChecked() || item->isHidden())
           && !_s_unfilt.contains(item->extension());
}

bool WidgetFileTypes::isPassed(CheckState state, const ItemFileType *item) const
{
    return (state == Checked && isPassedChecked(item))
           || (state == UnChecked && isPassedUnChecked(item));
}

bool WidgetFileTypes::itemsContain(CheckState state) const
{
    for (const ItemFileType *item : std::as_const(items_)) {
        if (isPassed(state, item)) {
            return true;
        }
    }

    return false;
}

bool WidgetFileTypes::hasChecked() const
{
    return itemsContain(Checked);
}

void WidgetFileTypes::showAllItems()
{
    for (int i = 0; i < topLevelItemCount(); ++i) {
        topLevelItem(i)->setHidden(false);
    }
}

void WidgetFileTypes::hideExtra(int nomore)
{
    ItemFileType::Column _sortColumn = (sortColumn() == ItemFileType::ColumnFilesNumber)
                                             ? ItemFileType::ColumnFilesNumber
                                             : ItemFileType::ColumnTotalSize;

    sortItems(_sortColumn, Qt::DescendingOrder);

    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *_item = topLevelItem(i);
        _item->setHidden(i >= nomore);
    }
}

NumSize WidgetFileTypes::numSizeVisible() const
{
    NumSize _res;

    for (int i = 0; i < topLevelItemCount(); ++i) {
        const ItemFileType *_item = static_cast<ItemFileType*>(topLevelItem(i));
        if (!_item->isHidden()) {
            _res += _item->numSize();
        }
    }

    return _res;
}

NumSize WidgetFileTypes::numSize(CheckState chk_state) const
{
    const QList<ItemFileType*> itemList = items(chk_state);
    NumSize _res;

    for (const ItemFileType *_item : itemList) {
        _res += _item->numSize();
    }

    return _res;
}

void WidgetFileTypes::setChecked(const QStringList &exts)
{
    setChecked(QSet<QString>(exts.begin(), exts.end()));
}

void WidgetFileTypes::setChecked(const QSet<QString> &exts)
{
    for (ItemFileType *_item : std::as_const(items_)) {
        if (_item->isCheckBoxVisible())
            _item->setChecked(exts.contains(_item->extension()));
    }
}
