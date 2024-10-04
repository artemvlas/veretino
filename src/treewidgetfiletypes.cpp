#include "treewidgetfiletypes.h"

TreeWidgetFileTypes::TreeWidgetFileTypes(QWidget *parent)
    : QTreeWidget(parent)
{
    icons_.setTheme(palette());
}

void TreeWidgetFileTypes::setItems(const QList<ExtNumSize> &extList)
{
    for (int i = 0; i < extList.size(); ++i) {
        QIcon icon;
        const QString _ext = extList.at(i).extension;

        if (_ext == ExtNumSize::strVeretinoDb)
            icon = icons_.icon(Icons::Database);
        else if (_ext == ExtNumSize::strShaFiles)
            icon = icons_.icon(Icons::HashFile);
        else if (_ext == ExtNumSize::strNoPerm)
            icon = icons_.icon(FileStatus::UnPermitted);
        else
            icon = icons_.icon(QStringLiteral(u"file.") + _ext);

        TreeWidgetItem *item = new TreeWidgetItem(this);
        item->setData(TreeWidgetItem::ColumnType, Qt::DisplayRole, _ext);
        item->setData(TreeWidgetItem::ColumnFilesNumber, Qt::DisplayRole, extList.at(i).filesNumber);
        item->setData(TreeWidgetItem::ColumnTotalSize, Qt::DisplayRole, format::dataSizeReadable(extList.at(i).filesSize));
        item->setData(TreeWidgetItem::ColumnTotalSize, Qt::UserRole, extList.at(i).filesSize);
        item->setIcon(TreeWidgetItem::ColumnType, icon);
        items_.append(item);
    }
}

void TreeWidgetFileTypes::setCheckboxesVisible(bool visible)
{
    static const QStringList excluded { ExtNumSize::strNoType, ExtNumSize::strVeretinoDb, ExtNumSize::strShaFiles, ExtNumSize::strNoPerm };

    this->blockSignals(true); // to avoid multiple calls &QTreeWidget::itemChanged --> ::updateFilterDisplay

    for (TreeWidgetItem *item : std::as_const(items_)) {
        item->setCheckBoxVisible(visible
                                 && !excluded.contains(item->extension()));
    }

    this->blockSignals(false);
}

QList<TreeWidgetItem *> TreeWidgetFileTypes::items(CheckState state) const
{
    QList<TreeWidgetItem *> resultList;

    for (TreeWidgetItem *item : std::as_const(items_)) {
        if (isPassed(state, item))
            resultList.append(item);
    }

    return resultList;
}

QStringList TreeWidgetFileTypes::checkedExtensions() const
{
    QStringList extensions;
    const QList<TreeWidgetItem *> checked_items = items(Checked);

    for (const TreeWidgetItem *item : checked_items) {
        extensions.append(item->extension());
    }

    return extensions;
}

bool TreeWidgetFileTypes::isPassedChecked(const TreeWidgetItem *item) const
{
    // allow only visible_checked
    return !item->isHidden() && item->isChecked();
}

bool TreeWidgetFileTypes::isPassedUnChecked(const TreeWidgetItem *item) const
{
    static const QStringList unfilterable { ExtNumSize::strVeretinoDb, ExtNumSize::strShaFiles, ExtNumSize::strNoPerm };

    // allow all except visible_checked and Db-Sha
    return (!item->isChecked() || item->isHidden())
           && !unfilterable.contains(item->extension());
}

bool TreeWidgetFileTypes::isPassed(CheckState state, const TreeWidgetItem *item) const
{
    return (state == Checked && isPassedChecked(item))
           || (state == UnChecked && isPassedUnChecked(item));
}

bool TreeWidgetFileTypes::itemsContain(CheckState state) const
{
    for (const TreeWidgetItem *item : std::as_const(items_)) {
        if (isPassed(state, item)) {
            return true;
        }
    }

    return false;
}

void TreeWidgetFileTypes::showAllItems()
{
    for (int i = 0; i < topLevelItemCount(); ++i)
        topLevelItem(i)->setHidden(false);
}

void TreeWidgetFileTypes::hideExtra(int nomore)
{
    TreeWidgetItem::Column _sortColumn = (sortColumn() == TreeWidgetItem::ColumnFilesNumber)
                                             ? TreeWidgetItem::ColumnFilesNumber
                                             : TreeWidgetItem::ColumnTotalSize;

    sortItems(_sortColumn, Qt::DescendingOrder);

    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *_item = topLevelItem(i);
        _item->setHidden(i >= nomore);
    }
}

NumSize TreeWidgetFileTypes::numSizeVisible()
{
    NumSize _nums;

    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *_item = topLevelItem(i);
        if (!_item->isHidden()) {
            _nums.num += _item->data(TreeWidgetItem::ColumnFilesNumber, Qt::DisplayRole).toInt();
            _nums.size += _item->data(TreeWidgetItem::ColumnTotalSize, Qt::UserRole).toLongLong();
        }
    }

    return _nums;
}

NumSize TreeWidgetFileTypes::numSize(CheckState chk_state)
{
    const QList<TreeWidgetItem *> itemList = items(chk_state);
    NumSize _res;

    for (const TreeWidgetItem *_item : itemList) {
        _res.add(_item->filesNumber(),
                 _item->filesSize());
    }

    return _res;
}
