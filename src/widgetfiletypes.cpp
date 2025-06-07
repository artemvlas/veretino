/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "widgetfiletypes.h"
#include "tools.h"

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
            QIcon icn;
            QString type;

            switch (it.key()) {
            case FilterAttribute::IgnoreDbFiles:
                icn = m_icons.icon(Icons::Database);
                type = QStringLiteral(u"Veretino DB");
                break;
            case FilterAttribute::IgnoreDigestFiles:
                icn = m_icons.icon(Icons::HashFile);
                type = QStringLiteral(u"sha1/256/512");
                break;
            case FilterAttribute::IgnoreUnpermitted:
                icn = m_icons.icon(FileStatus::UnPermitted);
                type = QStringLiteral(u"No Permissions");
                break;
            case FilterAttribute::IgnoreSymlinks:
                type = QStringLiteral(u"SymLinks");
                break;
            default:
                type = "Undefined";
                break;
            }

            addItem(type, it.value(), icn, (TypeAttribute::UnCheckable | TypeAttribute::UnFilterable));
        }
    }
}

ItemFileType* WidgetFileTypes::addItem(const QString &type, const NumSize &nums, const QIcon &icon, int attribute)
{
    ItemFileType *item = new ItemFileType(this);

    // ColumnType (0)
    item->setIcon(ItemFileType::ColumnType, icon);
    item->setData(ItemFileType::ColumnType, Qt::DisplayRole, type);
    if (attribute) // TypeAttribute
        item->setAttribute(attribute);

    // ColumnFilesNumber (1)
    item->setData(ItemFileType::ColumnFilesNumber, Qt::DisplayRole, nums._num);

    // ColumnTotalSize (2)
    item->setData(ItemFileType::ColumnTotalSize, Qt::DisplayRole, format::dataSizeReadable(nums._size));
    item->setData(ItemFileType::ColumnTotalSize, Qt::UserRole, nums._size);

    m_items.append(item);
    return item;
}

void WidgetFileTypes::setCheckboxesVisible(bool visible)
{
    this->blockSignals(true); // to avoid multiple calls &QTreeWidget::itemChanged --> ::updateFilterDisplay

    for (ItemFileType *it : std::as_const(m_items)) {
        it->setCheckBoxVisible(visible
                               && !it->hasAttribute(TypeAttribute::UnCheckable));
    }

    this->blockSignals(false);
}

QList<ItemFileType*> WidgetFileTypes::items(CheckState state) const
{
    QList<ItemFileType*> res;

    for (ItemFileType *it : std::as_const(m_items)) {
        if (isPassed(state, it))
            res.append(it);
    }

    return res;
}

QStringList WidgetFileTypes::checkedExtensions() const
{
    QStringList exts;
    const QList<ItemFileType*> checked_items = items(Checked);

    for (const ItemFileType *it : checked_items) {
        exts.append(it->extension());
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
    ItemFileType::Column sortCol = (sortColumn() == ItemFileType::ColumnFilesNumber)
                                       ? ItemFileType::ColumnFilesNumber
                                       : ItemFileType::ColumnTotalSize;

    sortItems(sortCol, Qt::DescendingOrder);

    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = topLevelItem(i);
        it->setHidden(i >= max_visible);
    }
}

NumSize WidgetFileTypes::numSizeVisible() const
{
    NumSize res;

    for (int i = 0; i < topLevelItemCount(); ++i) {
        const ItemFileType *it = static_cast<ItemFileType*>(topLevelItem(i));
        if (!it->isHidden()) {
            res += it->numSize();
        }
    }

    return res;
}

NumSize WidgetFileTypes::numSize(CheckState chk_state) const
{
    const QList<ItemFileType*> itemList = items(chk_state);
    NumSize res;

    for (const ItemFileType *it : itemList) {
        res += it->numSize();
    }

    return res;
}

void WidgetFileTypes::setChecked(const QStringList &exts)
{
    setChecked(QSet<QString>(exts.begin(), exts.end()));
}

void WidgetFileTypes::setChecked(const QSet<QString> &exts)
{
    for (ItemFileType *it : std::as_const(m_items)) {
        if (it->isCheckBoxVisible())
            it->setChecked(exts.contains(it->extension()));
    }
}
