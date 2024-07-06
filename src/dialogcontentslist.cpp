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

    QString folderName = paths::shortenPath(folderPath);
    ui->labelFolderName->setText(folderName);
    ui->labelFolderName->setToolTip(folderPath);
    ui->chbTop10->setVisible(extList.size() > 15);

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

    connect(ui->chbTop10, &QCheckBox::toggled, this, &DialogContentsList::setItemsVisibility);

    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &DialogContentsList::updateFilterDisplay);
    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, this, &DialogContentsList::activateItem);

    connect(ui->chbCreateFilter, &QCheckBox::toggled, this,
            [=](bool isChecked){ isChecked ? enableFilterCreating() : setFilterCreation(FC_Disabled); });

    connect(ui->rbIgnore, &QRadioButton::toggled, this, &DialogContentsList::updateFilterDisplay);

    connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &DialogContentsList::clearChecked);

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
        ui->chbTop10->setText("Top10");

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

        ui->chbTop10->setText(QString("Top10: %1 files (%2)")
                                    .arg(top10FilesNumber)
                                    .arg(format::dataSizeReadable(top10FilesSize)));
    }

    updateFilterDisplay();
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

    ui->treeWidget->blockSignals(true); // to avoid multiple calls &QTreeWidget::itemChanged --> ::updateFilterDisplay

    for (TreeWidgetItem *item : qAsConst(items_)) {
        item->setCheckBoxVisible(visible
                                 && !excluded.contains(item->extension()));
    }

    ui->treeWidget->blockSignals(false);
    updateFilterDisplay();

    qDebug() << Q_FUNC_INFO;
}

void DialogContentsList::clearChecked()
{
    if (mode_ == FC_Enabled) {
        ui->rbIgnore->setChecked(true);
        setCheckboxesVisible(true);
    }
}

void DialogContentsList::enableFilterCreating()
{
    setFilterCreation(FC_Enabled);
    ui->rbIgnore->setChecked(true);
    //setCheckboxesVisible(true);

    if (geometry().height() < 450 && geometry().x() > 0) // geometry().x() == 0 if the function is called from the constructor
        setGeometry(geometry().x(), geometry().y(), geometry().width(), 450);
}

void DialogContentsList::activateItem(QTreeWidgetItem *t_item)
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

bool DialogContentsList::isPassedChecked(const TreeWidgetItem *item) const
{
    // allow only visible_checked
    return !item->isHidden() && item->isChecked();
}

bool DialogContentsList::isPassedUnChecked(const TreeWidgetItem *item) const
{
    static const QStringList unfilterable { ExtNumSize::strVeretinoDb, ExtNumSize::strShaFiles };

    // allow all except visible_checked and Db-Sha
    return (!item->isChecked() || item->isHidden())
           && !unfilterable.contains(item->extension());
}

bool DialogContentsList::isPassed(CheckState state, const TreeWidgetItem *item) const
{
    return (state == Checked && isPassedChecked(item))
           || (state == UnChecked && isPassedUnChecked(item));
}

bool DialogContentsList::itemsContain(CheckState state) const
{
    if (mode_ != FC_Enabled)
        return false;

    bool contains = false;

    for (const TreeWidgetItem *item : qAsConst(items_)) {
        if (isPassed(state, item)) {
            contains = true;
            break;
        }
    }

    return contains;
}

QList<TreeWidgetItem *> DialogContentsList::items(CheckState state) const
{
    QList<TreeWidgetItem *> resultList;

    if (mode_ != FC_Enabled)
        return resultList;

    for (TreeWidgetItem *item : qAsConst(items_)) {
        if (isPassed(state, item))
            resultList.append(item);
    }

    return resultList;
}

QStringList DialogContentsList::checkedExtensions() const
{
    QStringList extensions;
    const QList<TreeWidgetItem *> checked_items = items(Checked);

    for (const TreeWidgetItem *item : checked_items) {
        extensions.append(item->extension());
    }

    return extensions;
}

void DialogContentsList::updateFilterDisplay()
{
    updateLabelFilterExtensions();
    updateLabelTotalFiltered();

    bool isFiltered = ui->rbInclude->isChecked() ? itemsContain(Checked)
                                                 : itemsContain(Checked) && itemsContain(UnChecked);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isFiltered);
}

void DialogContentsList::updateLabelFilterExtensions()
{
    if (mode_ != FC_Enabled) {
        ui->labelFilterExtensions->clear();
        return;
    }

    ui->labelFilterExtensions->setStyleSheet(format::coloredText(ui->rbIgnore->isChecked()));
    ui->labelFilterExtensions->setText(checkedExtensions().join(", "));
}

void DialogContentsList::updateLabelTotalFiltered()
{
    if (mode_ != FC_Enabled) {
        ui->labelTotalFiltered->clear();
        return;
    }

    // ? Include only visible_checked : Include all except visible_checked and Db-Sha
    const QList<TreeWidgetItem *> itemList = items(ui->rbInclude->isChecked() ? Checked : UnChecked);

    int filteredFilesNumber = 0;
    qint64 filteredFilesSize = 0;

    for (const TreeWidgetItem *item : itemList) {
        filteredFilesNumber += item->filesNumber();
        filteredFilesSize += item->filesSize();
    }

    if (itemsContain(Checked)) {
        ui->labelTotalFiltered->setText(QString("Filtered: %1")
                                        .arg(format::filesNumberAndSize(filteredFilesNumber, filteredFilesSize)));
    }
    else
        ui->labelTotalFiltered->clear();
}

FilterRule DialogContentsList::resultFilter()
{
    // (mode_ == FC_Enabled)
    FilterRule::FilterMode filterType = ui->rbIgnore->isChecked() ? FilterRule::Ignore : FilterRule::Include;
    return FilterRule(filterType, checkedExtensions());
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
    ui->chbCreateFilter->setChecked(mode_ == FC_Enabled);

    // the isVisible() condition is used to prevent an unnecessary call when opening the Dialog with FC_Disabled mode
    if (mode_ == FC_Enabled || (isVisible() && mode_ == FC_Disabled))
        setCheckboxesVisible(mode_ == FC_Enabled);

    // updateFilterDisplay();
}

void DialogContentsList::showEvent(QShowEvent *event)
{
    if (mode_ == FC_Hidden) // if the mode_ was set specifically by ::setFilterCreation(mode_ != FC_Hidden),
        updateViewMode(); // this function was already executed

    QDialog::showEvent(event);
}

void DialogContentsList::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        activateItem(ui->treeWidget->currentItem());
        return;
    }

    if (event->key() == Qt::Key_Escape && mode_ == FC_Enabled) {
        if (itemsContain(Checked))
            clearChecked();
        else
            setFilterCreation(FC_Disabled);
        return;
    }

    QDialog::keyPressEvent(event);
}
