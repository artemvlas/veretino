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
    m_icons.setTheme(palette());
}

void WidgetFileTypes::setItems(const FileTypeList &extList)
{
    if (!extList.m_extensions.isEmpty()) {
        QHash<QString, NumSize>::const_iterator it;
        for (it = extList.m_extensions.constBegin(); it != extList.m_extensions.constEnd(); ++it) {
            if (it.key().isEmpty()) {
                addItem(QStringLiteral(u"No type"), it.value(), QIcon(), TypeAttribute::UnCheckable);
            } else {
                addItem(it.key(), it.value(), m_icons.type_icon(it.key()));
            }
        }
    }

    if (!extList.m_combined.isEmpty()) {
        QHash<FilterAttribute, NumSize>::const_iterator it;
        for (it = extList.m_combined.constBegin(); it != extList.m_combined.constEnd(); ++it) {
            QIcon _icon;
            QString _type;

            switch (it.key()) {
            case FilterAttribute::IgnoreDbFiles:
                _icon = m_icons.icon(Icons::Database);
                _type = QStringLiteral(u"Veretino DB");
                break;
            case FilterAttribute::IgnoreDigestFiles:
                _icon = m_icons.icon(Icons::HashFile);
                _type = QStringLiteral(u"sha1/256/512");
                break;
            case FilterAttribute::IgnoreUnpermitted:
                _icon = m_icons.icon(FileStatus::UnPermitted);
                _type = QStringLiteral(u"No Permissions");
                break;
            case FilterAttribute::IgnoreSymlinks:
                _type = QStringLiteral(u"SymLinks");
                break;
            default:
                _type = "Undefined";
                break;
            }

            addItem(_type, it.value(), _icon, (TypeAttribute::UnCheckable | TypeAttribute::UnFilterable));
        }
    }
}

ItemFileType* WidgetFileTypes::addItem(const QString &type, const NumSize &nums, const QIcon &icon, int attribute)
{
    ItemFileType *_item = new ItemFileType(this);

    // ColumnType (0)
    _item->setIcon(ItemFileType::ColumnType, icon);
    _item->setData(ItemFileType::ColumnType, Qt::DisplayRole, type);
    if (attribute) // TypeAttribute
        _item->setAttribute(attribute);

    // ColumnFilesNumber (1)
    _item->setData(ItemFileType::ColumnFilesNumber, Qt::DisplayRole, nums._num);

    // ColumnTotalSize (2)
    _item->setData(ItemFileType::ColumnTotalSize, Qt::DisplayRole, format::dataSizeReadable(nums._size));
    _item->setData(ItemFileType::ColumnTotalSize, Qt::UserRole, nums._size);

    m_items.append(_item);
    return _item;
}

void WidgetFileTypes::setCheckboxesVisible(bool visible)
{
    this->blockSignals(true); // to avoid multiple calls &QTreeWidget::itemChanged --> ::updateFilterDisplay

    for (ItemFileType *_item : std::as_const(m_items)) {
        _item->setCheckBoxVisible(visible
                                  && !_item->hasAttribute(TypeAttribute::UnCheckable));
    }

    this->blockSignals(false);
}

QList<ItemFileType*> WidgetFileTypes::items(CheckState state) const
{
    QList<ItemFileType*> res;

    for (ItemFileType *_item : std::as_const(m_items)) {
        if (isPassed(state, _item))
            res.append(_item);
    }

    return res;
}

QStringList WidgetFileTypes::checkedExtensions() const
{
    QStringList exts;
    const QList<ItemFileType*> checked_items = items(Checked);

    for (const ItemFileType *_item : checked_items) {
        exts.append(_item->extension());
    }

    return exts;
}

bool WidgetFileTypes::isPassedChecked(const ItemFileType *item) const
{
    // allow only visible_checked
    return (!item->isHidden() && item->isChecked());
}

bool WidgetFileTypes::isPassedUnChecked(const ItemFileType *item) const
{
    // allow all except visible_checked and Db-Sha
    return (!item->isChecked() || item->isHidden())
           && !item->hasAttribute(TypeAttribute::UnFilterable);
}

bool WidgetFileTypes::isPassed(CheckState state, const ItemFileType *item) const
{
    return (state == Checked && isPassedChecked(item))
           || (state == UnChecked && isPassedUnChecked(item));
}

bool WidgetFileTypes::itemsContain(CheckState state) const
{
    for (const ItemFileType *item : std::as_const(m_items)) {
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

void WidgetFileTypes::hideExtra(int max_visible)
{
    ItemFileType::Column _sortColumn = (sortColumn() == ItemFileType::ColumnFilesNumber)
                                             ? ItemFileType::ColumnFilesNumber
                                             : ItemFileType::ColumnTotalSize;

    sortItems(_sortColumn, Qt::DescendingOrder);

    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *_item = topLevelItem(i);
        _item->setHidden(i >= max_visible);
    }
}

NumSize WidgetFileTypes::numSizeVisible() const
{
    NumSize res;

    for (int i = 0; i < topLevelItemCount(); ++i) {
        const ItemFileType *_item = static_cast<ItemFileType*>(topLevelItem(i));
        if (!_item->isHidden()) {
            res += _item->numSize();
        }
    }

    return res;
}

NumSize WidgetFileTypes::numSize(CheckState chk_state) const
{
    const QList<ItemFileType*> itemList = items(chk_state);
    NumSize res;

    for (const ItemFileType *_item : itemList) {
        res += _item->numSize();
    }

    return res;
}

void WidgetFileTypes::setChecked(const QStringList &exts)
{
    setChecked(QSet<QString>(exts.begin(), exts.end()));
}

void WidgetFileTypes::setChecked(const QSet<QString> &exts)
{
    for (ItemFileType *_item : std::as_const(m_items)) {
        if (_item->isCheckBoxVisible())
            _item->setChecked(exts.contains(_item->extension()));
    }
}
