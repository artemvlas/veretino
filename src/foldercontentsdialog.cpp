/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "foldercontentsdialog.h"
#include "ui_foldercontentsdialog.h"
#include "tools.h"
#include <QPushButton>
#include <QFileIconProvider>
#include <QDebug>

FolderContentsDialog::FolderContentsDialog(const QString &folderPath, const QList<ExtNumSize> &extList, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FolderContentsDialog)
    , extList_(extList)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/veretino.png"));
    ui->treeWidget->setColumnWidth(TreeWidgetItem::ColumnExtension, 130);
    ui->treeWidget->setColumnWidth(TreeWidgetItem::ColumnFilesNumber, 130);
    ui->treeWidget->sortByColumn(TreeWidgetItem::ColumnTotalSize, Qt::DescendingOrder);

    ui->rbIgnore->setVisible(false);
    ui->rbInclude->setVisible(false);
    ui->frameFilterExtensions->setVisible(false);
    ui->labelTotalFiltered->setVisible(false);
    ui->buttonBox->setVisible(false);

    QString folderName = paths::isRoot(paths::parentFolder(folderPath)) ? folderPath
                                                                        : ".../" + paths::basicName(folderPath);
    ui->labelFolderName->setText(folderName);
    ui->labelFolderName->setToolTip(folderPath);
    ui->checkBox_Top10->setVisible(extList.size() > 15);

    setTotalInfo();
    makeItemsList(extList);
    connections();
}

FolderContentsDialog::~FolderContentsDialog()
{
    delete ui;
}

void FolderContentsDialog::connections()
{
    connect(ui->checkBox_Top10, &QCheckBox::toggled, this, &FolderContentsDialog::setItemsVisibility);

    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &FolderContentsDialog::updateFilterExtensionsList);
    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, this, &FolderContentsDialog::handleDoubleClickedItem);

    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, ui->rbIgnore, &QRadioButton::setVisible);
    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, ui->rbInclude, &QRadioButton::setVisible);
    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, ui->frameFilterExtensions, &QFrame::setVisible);
    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, ui->labelTotalFiltered, &QLabel::setVisible);
    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, ui->buttonBox, &QDialogButtonBox::setVisible);
    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, this, [=](bool isChecked){isChecked ? enableFilterCreating() : disableFilterCreating();});

    connect(ui->rbIgnore, &QRadioButton::toggled, this, &FolderContentsDialog::updateTotalFiltered);
    connect(ui->rbIgnore, &QRadioButton::toggled, this, &FolderContentsDialog::updateFilterExtensionsList);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &FolderContentsDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &FolderContentsDialog::reject);
    connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &FolderContentsDialog::enableFilterCreating);

    connect(ui->labelFolderName, &ClickableLabel::doubleClicked, this, [=]{paths::browsePath(ui->labelFolderName->toolTip());});
}

void FolderContentsDialog::makeItemsList(const QList<ExtNumSize> &extList)
{
    QFileIconProvider fileIcons;

    for (int i = 0; i < extList.size(); ++i) {
        TreeWidgetItem *item = new TreeWidgetItem(ui->treeWidget);
        item->setData(TreeWidgetItem::ColumnExtension, Qt::DisplayRole, extList.at(i).extension);
        item->setData(TreeWidgetItem::ColumnFilesNumber, Qt::DisplayRole, extList.at(i).filesNumber);
        item->setData(TreeWidgetItem::ColumnTotalSize, Qt::DisplayRole, format::dataSizeReadable(extList.at(i).filesSize));
        item->setData(TreeWidgetItem::ColumnTotalSize, Qt::UserRole, extList.at(i).filesSize);
        item->setIcon(TreeWidgetItem::ColumnExtension, fileIcons.icon(QFileInfo("file." + item->extension())));
        items.append(item);
    }
}

void FolderContentsDialog::setItemsVisibility(bool isTop10Checked)
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

void FolderContentsDialog::setTotalInfo()
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

void FolderContentsDialog::enableFilterCreating()
{
    ui->rbIgnore->setChecked(true);
    filterExtensions.clear();
    ui->labelFilterExtensions->clear();
    ui->labelTotalFiltered->clear();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);

    for (int i = 0; i < items.size(); ++i) {
        if (items.at(i)->extension() != "No type")
            items.at(i)->setCheckBoxVisible(true);
    }

    if (geometry().height() < 450 && geometry().x() > 0) // geometry().x() == 0 if the function is called from the constructor
        setGeometry(geometry().x(), geometry().y(), geometry().width(), 450);
}

void FolderContentsDialog::disableFilterCreating()
{
    ui->labelFilterExtensions->clear();
    filterExtensions.clear();

    for (int i = 0; i < items.size(); ++i) {
        items.at(i)->setCheckBoxVisible(false);
    }
}

void FolderContentsDialog::handleDoubleClickedItem(QTreeWidgetItem *item)
{
    if (!isFilterCreatingEnabled()) {
        setFilterCreatingEnabled();
        return;
    }

    if (!item->data(TreeWidgetItem::ColumnExtension, Qt::CheckStateRole).isValid())
        return;

    Qt::CheckState checkState = (item->checkState(TreeWidgetItem::ColumnExtension) == Qt::Unchecked) ? Qt::Checked : Qt::Unchecked;
    item->setCheckState(TreeWidgetItem::ColumnExtension, checkState);
}

void FolderContentsDialog::updateFilterExtensionsList()
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

void FolderContentsDialog::updateTotalFiltered()
{
    if (!isFilterCreatingEnabled())
        return;

    int filteredFilesNumber = 0;
    qint64 filteredFilesSize = 0;

    for (int i = 0; i < items.size(); ++i) {
        if ((ui->rbInclude->isChecked() && !items.at(i)->isHidden() && items.at(i)->isChecked()) // Include only visible and checked
            || (ui->rbIgnore->isChecked() && (items.at(i)->isHidden() || !items.at(i)->isChecked()))) { // Include all except visible and checked

            filteredFilesNumber += items.at(i)->filesNumber();
            filteredFilesSize += items.at(i)->filesSize();
        }
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!filterExtensions.isEmpty() && filteredFilesNumber > 0);

    filterExtensions.isEmpty() ? ui->labelTotalFiltered->clear()
                               : ui->labelTotalFiltered->setText(QString("Filtered: %1")
                                                                 .arg(format::filesNumberAndSize(filteredFilesNumber, filteredFilesSize)));
}

FilterRule FolderContentsDialog::resultFilter()
{
    if (isFilterCreatingEnabled() && !filterExtensions.isEmpty()) {
        FilterRule::ExtensionsFilter filterType = ui->rbIgnore->isChecked() ? FilterRule::Ignore : FilterRule::Include;
        return FilterRule(filterType, filterExtensions);
    }

    return FilterRule(true);
}

void FolderContentsDialog::setFilterCreatingEnabled(bool enabled)
{
    ui->checkBox_CreateFilter->setChecked(enabled);
}

bool FolderContentsDialog::isFilterCreatingEnabled()
{
    return ui->checkBox_CreateFilter->isChecked();
}
