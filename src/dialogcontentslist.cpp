/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogcontentslist.h"
#include "ui_dialogcontentslist.h"
#include "tools.h"
#include <QPushButton>
#include <QDebug>

DialogContentsList::DialogContentsList(const QString &folderPath, const QList<ExtNumSize> &extList, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogContentsList)
    , extList_(extList)
{
    ui->setupUi(this);
    icons_.setTheme(palette());
    setWindowIcon(icons_.iconFolder());
    ui->treeWidget->setColumnWidth(TreeWidgetItem::ColumnType, 130);
    ui->treeWidget->setColumnWidth(TreeWidgetItem::ColumnFilesNumber, 130);
    ui->treeWidget->sortByColumn(TreeWidgetItem::ColumnTotalSize, Qt::DescendingOrder);

    ui->rbIgnore->setVisible(false);
    ui->rbInclude->setVisible(false);
    ui->frameFilterExtensions->setVisible(false);
    ui->labelTotalFiltered->setVisible(false);
    ui->buttonBox->setVisible(false);
    ui->labelFilterExtensions->clear();

    QString folderName = paths::isRoot(paths::parentFolder(folderPath)) ? folderPath
                                                                        : ".../" + paths::basicName(folderPath);
    ui->labelFolderName->setText(folderName);
    ui->labelFolderName->setToolTip(folderPath);
    ui->checkBox_Top10->setVisible(extList.size() > 15);

    setTotalInfo();
    makeItemsList(extList);
    connections();
}

DialogContentsList::~DialogContentsList()
{
    delete ui;
}

void DialogContentsList::connections()
{
    connect(ui->checkBox_Top10, &QCheckBox::toggled, this, &DialogContentsList::setItemsVisibility);

    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &DialogContentsList::updateFilterExtensionsList);
    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, this, &DialogContentsList::handleDoubleClickedItem);

    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, ui->rbIgnore, &QRadioButton::setVisible);
    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, ui->rbInclude, &QRadioButton::setVisible);
    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, ui->frameFilterExtensions, &QFrame::setVisible);
    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, ui->labelTotalFiltered, &QLabel::setVisible);
    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, ui->buttonBox, &QDialogButtonBox::setVisible);
    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, this,
            [=](bool isChecked){ isChecked ? enableFilterCreating() : disableFilterCreating(); });

    connect(ui->rbIgnore, &QRadioButton::toggled, this, &DialogContentsList::updateTotalFiltered);
    connect(ui->rbIgnore, &QRadioButton::toggled, this, &DialogContentsList::updateFilterExtensionsList);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogContentsList::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &DialogContentsList::reject);
    connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &DialogContentsList::enableFilterCreating);

    connect(ui->labelFolderName, &ClickableLabel::doubleClicked, this, [=]{ paths::browsePath(ui->labelFolderName->toolTip()); });
}

void DialogContentsList::makeItemsList(const QList<ExtNumSize> &extList)
{
    for (int i = 0; i < extList.size(); ++i) {
        QIcon icon;

        if (extList.at(i).extension == ExtNumSize::strVeretinoDb)
            icon = icons_.icon(Icons::Database);
        else if (extList.at(i).extension == ExtNumSize::strShaFiles)
            icon = icons_.icon(Icons::HashFile);
        else
            icon = icons_.icon("file." + extList.at(i).extension);

        TreeWidgetItem *item = new TreeWidgetItem(ui->treeWidget);
        item->setData(TreeWidgetItem::ColumnType, Qt::DisplayRole, extList.at(i).extension);
        item->setData(TreeWidgetItem::ColumnFilesNumber, Qt::DisplayRole, extList.at(i).filesNumber);
        item->setData(TreeWidgetItem::ColumnTotalSize, Qt::DisplayRole, format::dataSizeReadable(extList.at(i).filesSize));
        item->setData(TreeWidgetItem::ColumnTotalSize, Qt::UserRole, extList.at(i).filesSize);
        item->setIcon(TreeWidgetItem::ColumnType, icon);
        items.append(item);
    }
}

void DialogContentsList::setItemsVisibility(bool isTop10Checked)
{
    if (!isTop10Checked) {
        ui->checkBox_Top10->setText("Top10");

        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
            ui->treeWidget->topLevelItem(i)->setHidden(false);
    }
    else {
        if (ui->treeWidget->sortColumn() == TreeWidgetItem::ColumnFilesNumber)
            ui->treeWidget->sortItems(TreeWidgetItem::ColumnFilesNumber, Qt::DescendingOrder);
        else
            ui->treeWidget->sortItems(TreeWidgetItem::ColumnTotalSize, Qt::DescendingOrder);

        int top10FilesNumber = 0; // total number of files in the Top10 list
        qint64 top10FilesSize = 0; // total size of these files

        for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem *item = ui->treeWidget->topLevelItem(i);
            item->setHidden(i > 9);
            if (!item->isHidden()) {
                top10FilesNumber += item->data(TreeWidgetItem::ColumnFilesNumber, Qt::DisplayRole).toInt();
                top10FilesSize += item->data(TreeWidgetItem::ColumnTotalSize, Qt::UserRole).toLongLong();
            }
        }

        ui->checkBox_Top10->setText(QString("Top10: %1 files (%2)")
                                            .arg(top10FilesNumber)
                                            .arg(format::dataSizeReadable(top10FilesSize)));
    }

    updateFilterExtensionsList();
}

void DialogContentsList::setTotalInfo()
{
    qint64 totalSize = 0;
    int totalFilesNumber = 0;

    for (int i = 0; i < extList_.size(); ++i) {
        totalSize += extList_.at(i).filesSize;
        totalFilesNumber += extList_.at(i).filesNumber;
    }

    ui->labelTotal->setText(QString("Total: %1 types, %2 files (%3) ")
                                .arg(extList_.size())
                                .arg(totalFilesNumber)
                                .arg(format::dataSizeReadable(totalSize)));
}

void DialogContentsList::enableFilterCreating()
{
    ui->rbIgnore->setChecked(true);
    filterExtensions.clear();
    ui->labelFilterExtensions->clear();
    ui->labelTotalFiltered->clear();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    const QStringList excluded({ExtNumSize::strNoType, ExtNumSize::strVeretinoDb, ExtNumSize::strShaFiles});

    for (int i = 0; i < items.size(); ++i) {
        if (!excluded.contains(items.at(i)->extension()))
            items.at(i)->setCheckBoxVisible(true);
    }

    if (geometry().height() < 450 && geometry().x() > 0) // geometry().x() == 0 if the function is called from the constructor
        setGeometry(geometry().x(), geometry().y(), geometry().width(), 450);
}

void DialogContentsList::disableFilterCreating()
{
    ui->labelFilterExtensions->clear();
    filterExtensions.clear();

    for (int i = 0; i < items.size(); ++i) {
        items.at(i)->setCheckBoxVisible(false);
    }
}

void DialogContentsList::handleDoubleClickedItem(QTreeWidgetItem *item)
{
    if (mode_ == FC_Hidden)
        return;

    if (!isFilterCreatingEnabled()) {
        setFilterCreation(DialogContentsList::FC_Enabled);
        return;
    }

    if (!item->data(TreeWidgetItem::ColumnType, Qt::CheckStateRole).isValid())
        return;

    Qt::CheckState checkState = (item->checkState(TreeWidgetItem::ColumnType) == Qt::Unchecked) ? Qt::Checked : Qt::Unchecked;
    item->setCheckState(TreeWidgetItem::ColumnType, checkState);
}

void DialogContentsList::updateFilterExtensionsList()
{
    if (!isFilterCreatingEnabled())
        return;

    filterExtensions.clear();

    for (int i = 0; i < items.size(); ++i) {
        if (!items.at(i)->isHidden() && items.at(i)->isChecked()) {
            filterExtensions.append(items.at(i)->extension());
        }
    }

    ui->labelFilterExtensions->setStyleSheet(format::coloredText(ui->rbIgnore->isChecked()));
    ui->labelFilterExtensions->setText(filterExtensions.join(" "));

    updateTotalFiltered();
}

void DialogContentsList::updateTotalFiltered()
{
    if (!isFilterCreatingEnabled())
        return;

    int filteredFilesNumber = 0;
    qint64 filteredFilesSize = 0;

    for (int i = 0; i < items.size(); ++i) {
        TreeWidgetItem *item = items.at(i);
        if ((ui->rbInclude->isChecked() && !item->isHidden() && item->isChecked()) // Include only visible and checked
            || (ui->rbIgnore->isChecked() && isItemFilterable(item) && (item->isHidden() || !item->isChecked()))) { // Include all except visible and checked

            filteredFilesNumber += item->filesNumber();
            filteredFilesSize += item->filesSize();
        }
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!filterExtensions.isEmpty() && filteredFilesNumber > 0);

    filterExtensions.isEmpty() ? ui->labelTotalFiltered->clear()
                               : ui->labelTotalFiltered->setText(QString("Filtered: %1")
                                                                 .arg(format::filesNumberAndSize(filteredFilesNumber, filteredFilesSize)));
}

FilterRule DialogContentsList::resultFilter()
{
    if (isFilterCreatingEnabled() && !filterExtensions.isEmpty()) {
        FilterRule::FilterMode filterType = ui->rbIgnore->isChecked() ? FilterRule::Ignore : FilterRule::Include;
        return FilterRule(filterType, filterExtensions);
    }

    return FilterRule(true);
}

void DialogContentsList::setFilterCreation(FilterCreation mode)
{
    mode_ = mode;
    updateViewMode();
}

void DialogContentsList::updateViewMode()
{
    ui->frameCreateFilter->setVisible(mode_ != FC_Hidden);
    ui->frameFilterExtensions->setVisible(mode_ != FC_Hidden);
    ui->checkBox_CreateFilter->setChecked(mode_ == FC_Enabled);
}

bool DialogContentsList::isFilterCreatingEnabled()
{
    return ui->checkBox_CreateFilter->isChecked();
}

bool DialogContentsList::isItemFilterable(const TreeWidgetItem *item)
{
    return ((item->extension() != ExtNumSize::strVeretinoDb) && (item->extension() != ExtNumSize::strShaFiles));
}
