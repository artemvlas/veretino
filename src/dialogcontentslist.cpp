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

    QString folderName = paths::isRoot(paths::parentFolder(folderPath)) ? folderPath
                                                                        : "../" + paths::basicName(folderPath);
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
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogContentsList::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &DialogContentsList::reject);

    connect(ui->checkBox_Top10, &QCheckBox::toggled, this, &DialogContentsList::setItemsVisibility);

    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &DialogContentsList::updateLabelFilterExtensions);
    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, this, &DialogContentsList::handleDoubleClickedItem);

    connect(ui->checkBox_CreateFilter, &QCheckBox::toggled, this,
            [=](bool isChecked){ isChecked ? enableFilterCreating() : disableFilterCreating(); });

    connect(ui->rbIgnore, &QRadioButton::toggled, this, &DialogContentsList::updateLabelTotalFiltered);
    connect(ui->rbIgnore, &QRadioButton::toggled, this, &DialogContentsList::updateLabelFilterExtensions);

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
        items_.append(item);
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

    updateLabelFilterExtensions();
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

void DialogContentsList::setCheckboxesVisible(bool visible)
{
    static const QStringList excluded { ExtNumSize::strNoType, ExtNumSize::strVeretinoDb, ExtNumSize::strShaFiles };

    for (int i = 0; i < items_.size(); ++i) {
        items_.at(i)->setCheckBoxVisible(visible
                                         && !excluded.contains(items_.at(i)->extension()));
    }
}

void DialogContentsList::enableFilterCreating()
{
    setFilterCreation(FC_Enabled);
    ui->rbIgnore->setChecked(true);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    setCheckboxesVisible(true);

    if (geometry().height() < 450 && geometry().x() > 0) // geometry().x() == 0 if the function is called from the constructor
        setGeometry(geometry().x(), geometry().y(), geometry().width(), 450);
}

void DialogContentsList::disableFilterCreating()
{
    setFilterCreation(FC_Disabled);
    setCheckboxesVisible(false);
}

void DialogContentsList::handleDoubleClickedItem(QTreeWidgetItem *t_item)
{
    if (mode_ == FC_Hidden)
        return;

    if (mode_ == FC_Disabled) {
        setFilterCreation(FC_Enabled);
        return;
    }

    TreeWidgetItem *item = static_cast<TreeWidgetItem*>(t_item);
    item->toggle();
}

QStringList DialogContentsList::checkedExtensions()
{
    QStringList result;

    for (int i = 0; i < items_.size(); ++i) {
        if (!items_.at(i)->isHidden() && items_.at(i)->isChecked()) {
            result.append(items_.at(i)->extension());
        }
    }

    return result;
}

void DialogContentsList::updateLabelFilterExtensions()
{
    if (mode_ != FC_Enabled)
        return;

    filterExtensions_ = checkedExtensions();

    ui->labelFilterExtensions->setStyleSheet(format::coloredText(ui->rbIgnore->isChecked()));
    ui->labelFilterExtensions->setText(filterExtensions_.join(" "));

    updateLabelTotalFiltered();
}

void DialogContentsList::updateLabelTotalFiltered()
{
    if (mode_ != FC_Enabled)
        return;

    int filteredFilesNumber = 0;
    qint64 filteredFilesSize = 0;

    for (int i = 0; i < items_.size(); ++i) {
        TreeWidgetItem *item = items_.at(i);
        if ((ui->rbInclude->isChecked() && !item->isHidden() && item->isChecked()) // Include only visible and checked
            || (ui->rbIgnore->isChecked() && isItemFilterable(item) && (item->isHidden() || !item->isChecked()))) { // Include all except visible and checked

            filteredFilesNumber += item->filesNumber();
            filteredFilesSize += item->filesSize();
        }
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!filterExtensions_.isEmpty() && filteredFilesNumber > 0);

    filterExtensions_.isEmpty() ? ui->labelTotalFiltered->clear()
                                : ui->labelTotalFiltered->setText(QString("Filtered: %1")
                                                                  .arg(format::filesNumberAndSize(filteredFilesNumber, filteredFilesSize)));
}

FilterRule DialogContentsList::resultFilter()
{
    if ((mode_ == FC_Enabled) && !filterExtensions_.isEmpty()) {
        FilterRule::FilterMode filterType = ui->rbIgnore->isChecked() ? FilterRule::Ignore : FilterRule::Include;
        return FilterRule(filterType, filterExtensions_);
    }

    return FilterRule(true);
}

void DialogContentsList::setFilterCreation(FilterCreation mode)
{
    if (mode_ != mode) {
        mode_ = mode;
        updateViewMode();
    }
}

void DialogContentsList::updateViewMode()
{
    ui->rbIgnore->setVisible(mode_ == FC_Enabled);
    ui->rbInclude->setVisible(mode_ == FC_Enabled);
    ui->frameFilterExtensions->setVisible(mode_ == FC_Enabled);
    ui->labelTotalFiltered->setVisible(mode_ == FC_Enabled);
    ui->buttonBox->setVisible(mode_ == FC_Enabled);

    ui->frameCreateFilter->setVisible(mode_ != FC_Hidden);
    ui->checkBox_CreateFilter->setChecked(mode_ == FC_Enabled);

    ui->labelTotalFiltered->clear();
    ui->labelFilterExtensions->clear();
    filterExtensions_.clear();
}

bool DialogContentsList::isItemFilterable(const TreeWidgetItem *item)
{
    return ((item->extension() != ExtNumSize::strVeretinoDb) && (item->extension() != ExtNumSize::strShaFiles));
}

void DialogContentsList::showEvent(QShowEvent *event)
{
    // qDebug() << Q_FUNC_INFO;
    updateViewMode();
    QDialog::showEvent(event);
}
