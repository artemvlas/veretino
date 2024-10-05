#include "widgetfiletypes.h"

WidgetFileTypes::WidgetFileTypes(QWidget *parent)
    : QTreeWidget(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    icons_.setTheme(palette());
}

void WidgetFileTypes::setItems(const QList<ExtNumSize> &extList)
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

        ItemFileType *item = new ItemFileType(this);
        item->setData(ItemFileType::ColumnType, Qt::DisplayRole, _ext);
        item->setData(ItemFileType::ColumnFilesNumber, Qt::DisplayRole, extList.at(i).filesNumber);
        item->setData(ItemFileType::ColumnTotalSize, Qt::DisplayRole, format::dataSizeReadable(extList.at(i).filesSize));
        item->setData(ItemFileType::ColumnTotalSize, Qt::UserRole, extList.at(i).filesSize);
        item->setIcon(ItemFileType::ColumnType, icon);
        items_.append(item);
    }
}

void WidgetFileTypes::setCheckboxesVisible(bool visible)
{
    static const QStringList excluded { ExtNumSize::strNoType, ExtNumSize::strVeretinoDb, ExtNumSize::strShaFiles, ExtNumSize::strNoPerm };

    this->blockSignals(true); // to avoid multiple calls &QTreeWidget::itemChanged --> ::updateFilterDisplay

    for (ItemFileType *item : std::as_const(items_)) {
        item->setCheckBoxVisible(visible
                                 && !excluded.contains(item->extension()));
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
    QStringList extensions;
    const QList<ItemFileType *> checked_items = items(Checked);

    for (const ItemFileType *item : checked_items) {
        extensions.append(item->extension());
    }

    return extensions;
}

bool WidgetFileTypes::isPassedChecked(const ItemFileType *item) const
{
    // allow only visible_checked
    return !item->isHidden() && item->isChecked();
}

bool WidgetFileTypes::isPassedUnChecked(const ItemFileType *item) const
{
    static const QStringList unfilterable { ExtNumSize::strVeretinoDb, ExtNumSize::strShaFiles, ExtNumSize::strNoPerm };

    // allow all except visible_checked and Db-Sha
    return (!item->isChecked() || item->isHidden())
           && !unfilterable.contains(item->extension());
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

void WidgetFileTypes::showAllItems()
{
    for (int i = 0; i < topLevelItemCount(); ++i)
        topLevelItem(i)->setHidden(false);
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
    NumSize _nums;

    for (int i = 0; i < topLevelItemCount(); ++i) {
        const ItemFileType *_item = static_cast<ItemFileType*>(topLevelItem(i));
        if (!_item->isHidden()) {
            _nums.add(_item->filesNumber(), _item->filesSize());
        }
    }

    return _nums;
}

NumSize WidgetFileTypes::numSize(CheckState chk_state) const
{
    const QList<ItemFileType*> itemList = items(chk_state);
    NumSize _res;

    for (const ItemFileType *_item : itemList) {
        _res.add(_item->filesNumber(),
                 _item->filesSize());
    }

    return _res;
}

void WidgetFileTypes::setChecked(const QStringList &exts)
{
    for (ItemFileType *_item : std::as_const(items_)) {
        if (exts.contains(_item->extension()))
            _item->setChecked(true);
    }
}
