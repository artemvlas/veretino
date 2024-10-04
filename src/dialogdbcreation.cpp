#include "dialogdbcreation.h"
#include "ui_dialogdbcreation.h"
#include "tools.h"
#include <QPushButton>
#include "treewidgetfiletypes.h"
#include <QDebug>

DialogDbCreation::DialogDbCreation(const QString &folderPath, const QList<ExtNumSize> &extList, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogDbCreation)
    , extList_(extList)
    , workDir_(folderPath)
{
    ui->setupUi(this);

    icons_.setTheme(palette());
    setWindowIcon(icons_.iconFolder());
    ui->treeWidget->setColumnWidth(TreeWidgetItem::ColumnType, 130);
    ui->treeWidget->setColumnWidth(TreeWidgetItem::ColumnFilesNumber, 130);
    ui->treeWidget->sortByColumn(TreeWidgetItem::ColumnTotalSize, Qt::DescendingOrder);

    //QString folderName = paths::shortenPath(folderPath);
    //ui->labelFolderName->setText(folderName);
    //ui->labelFolderName->setToolTip(folderPath);
    ui->cb_top10->setVisible(extList.size() > 15);

    setTotalInfo();
    connections();
    ui->treeWidget->setItems(extList);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setIcon(icons_.icon(FileStatus::Calculating));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Continue");
}

DialogDbCreation::~DialogDbCreation()
{
    delete ui;
}

void DialogDbCreation::connections()
{
    //connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DialogDbCreation::accept);
    //connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &DialogDbCreation::reject);

    connect(ui->cb_top10, &QCheckBox::toggled, this, &DialogDbCreation::setItemsVisibility);

    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &DialogDbCreation::updateFilterDisplay);
    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, this, &DialogDbCreation::activateItem);

    connect(ui->cb_enable_filter, &QCheckBox::toggled, this,
            [=](bool isChecked){ setFilterCreation(isChecked ? FC_Enabled : FC_Disabled); });

    connect(ui->rb_ignore, &QRadioButton::toggled, this, &DialogDbCreation::updateFilterDisplay);

    connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &DialogDbCreation::clearChecked);

    // filename
    connect(ui->rb_ext_short, &QRadioButton::toggled, this, &DialogDbCreation::updateLabelDbFilename);
    connect(ui->cb_add_folder_name, &QCheckBox::toggled, this, &DialogDbCreation::updateLabelDbFilename);
    connect(ui->inp_db_filename, &QLineEdit::textEdited, this, &DialogDbCreation::updateLabelDbFilename);

    // settings
    connect(this, &DialogDbCreation::accepted, this, &DialogDbCreation::updateSettings);
}

void DialogDbCreation::setSettings(Settings *settings)
{
    settings_ = settings;
    setDbConfig();
}

void DialogDbCreation::updateSettings()
{
    if (!settings_)
        return;

    qDebug() << "DialogDbCreation::updateSettings()";

    settings_->isLongExtension = ui->rb_ext_long->isChecked();
    settings_->addWorkDirToFilename = ui->cb_add_folder_name->isChecked();
    settings_->dbFlagConst = ui->cb_flag_const->isChecked();
    settings_->dbPrefix = ui->inp_db_filename->text().isEmpty() ? QStringLiteral(u"checksums")
                                                                : format::simplifiedChars(ui->inp_db_filename->text());
}

void DialogDbCreation::setDbConfig()
{
    if (!settings_)
        return;

    ui->rb_ext_short->setChecked(!settings_->isLongExtension);
    ui->cb_add_folder_name->setChecked(settings_->addWorkDirToFilename);
    ui->cb_flag_const->setChecked(settings_->dbFlagConst);

    if (settings_->dbPrefix != QStringLiteral(u"checksums"))
        ui->inp_db_filename->setText(settings_->dbPrefix);
}

void DialogDbCreation::updateLabelDbFilename()
{
    QString prefix = ui->inp_db_filename->text().isEmpty() ? QStringLiteral(u"checksums") : format::simplifiedChars(ui->inp_db_filename->text());
    QString folderName = ui->cb_add_folder_name->isChecked() ? QStringLiteral(u"@FolderName") : QString();
    QString extension = Lit::sl_db_exts.at(ui->rb_ext_short->isChecked());

    ui->l_db_filename->setText(format::composeDbFileName(prefix, folderName, extension));
}

void DialogDbCreation::setItemsVisibility(bool isTop10Checked)
{
    if (!isTop10Checked) {
        ui->cb_top10->setText(QStringLiteral(u"Top10"));
        ui->treeWidget->showAllItems();
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

        ui->cb_top10->setText(QStringLiteral(u"Top10: ")
                              + format::filesNumSize(top10FilesNumber, top10FilesSize));
    }

    updateFilterDisplay();
}

void DialogDbCreation::setTotalInfo()
{
    qint64 totalSize = 0;
    int totalFilesNumber = 0;

    for (int i = 0; i < extList_.size(); ++i) {
        totalSize += extList_.at(i).filesSize;
        totalFilesNumber += extList_.at(i).filesNumber;
    }

    ui->l_total_files->setText(QString("Total: %1 types, %2 ")
                                .arg(extList_.size())
                                .arg(format::filesNumSize(totalFilesNumber, totalSize)));
}

void DialogDbCreation::setCheckboxesVisible(bool visible)
{
    ui->treeWidget->setCheckboxesVisible(visible);
    updateFilterDisplay();

    // qDebug() << Q_FUNC_INFO;
}

void DialogDbCreation::clearChecked()
{
    if (mode_ == FC_Enabled) {
        ui->rb_ignore->setChecked(true);
        setCheckboxesVisible(true);
    }
}

void DialogDbCreation::activateItem(QTreeWidgetItem *t_item)
{
    //qDebug() << "MODE" << mode_;
    //if (mode_ == FC_Hidden)
    //    return;

    if (mode_ == FC_Disabled) {
        setFilterCreation(FC_Enabled);
        return;
    }

    TreeWidgetItem *item = static_cast<TreeWidgetItem*>(t_item);
    item->toggle();
}


bool DialogDbCreation::itemsContain(int state) const
{
    if (mode_ != FC_Enabled)
        return false;

    return ui->treeWidget->itemsContain((TreeWidgetFileTypes::CheckState)state);
}

void DialogDbCreation::updateFilterDisplay()
{
    updateLabelFilterExtensions();
    updateLabelTotalFiltered();

    //bool isFiltered = ui->rb_include->isChecked() ? itemsContain(TreeWidgetFileTypes::Checked)
    //                                              : itemsContain(TreeWidgetFileTypes::Checked) && itemsContain(TreeWidgetFileTypes::UnChecked);

    //ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isFiltered);
}

void DialogDbCreation::updateLabelFilterExtensions()
{
    if (mode_ != FC_Enabled) {
        ui->l_exts_list->clear();
        return;
    }

    ui->l_exts_list->setStyleSheet(format::coloredText(ui->rb_ignore->isChecked()));
    ui->l_exts_list->setText(ui->treeWidget->checkedExtensions().join(QStringLiteral(u", ")));
}

void DialogDbCreation::updateLabelTotalFiltered()
{
    if (mode_ != FC_Enabled) {
        ui->l_total_filtered->clear();
        return;
    }

    // ? Include only visible_checked : Include all except visible_checked and Db-Sha
    TreeWidgetFileTypes::CheckState _checkState = ui->rb_include->isChecked() ? TreeWidgetFileTypes::Checked
                                                                              : TreeWidgetFileTypes::UnChecked;
    const QList<TreeWidgetItem *> itemList = ui->treeWidget->items(_checkState);

    int filteredFilesNumber = 0;
    qint64 filteredFilesSize = 0;

    for (const TreeWidgetItem *item : itemList) {
        filteredFilesNumber += item->filesNumber();
        filteredFilesSize += item->filesSize();
    }

    if (itemsContain(TreeWidgetFileTypes::Checked)) {
        ui->l_total_filtered->setText(QStringLiteral(u"Filtered: ")
                                        + format::filesNumSize(filteredFilesNumber, filteredFilesSize));
    }
    else {
        ui->l_total_filtered->clear();
        //ui->l_total_filtered->setText("No filter");
    }
}

FilterRule DialogDbCreation::resultFilter()
{
    FilterRule::FilterMode filterType = ui->rb_ignore->isChecked() ? FilterRule::Ignore : FilterRule::Include;
    return FilterRule(filterType, ui->treeWidget->checkedExtensions());
}

void DialogDbCreation::setFilterCreation(FilterCreation mode)
{
    if (mode_ != mode) {
        mode_ = mode;
        updateViewMode();
    }
}

void DialogDbCreation::updateViewMode()
{
    ui->rb_ignore->setVisible(mode_ == FC_Enabled);
    ui->rb_include->setVisible(mode_ == FC_Enabled);
    ui->frameFilterExtensions->setVisible(mode_ == FC_Enabled);
    //ui->l_total_filtered->setVisible(mode_ == FC_Enabled);

    //ui->frameCreateFilter->setVisible(mode_ != FC_Hidden);
    ui->cb_enable_filter->setChecked(mode_ == FC_Enabled);

    // the isVisible() condition is used to prevent an unnecessary call when opening the Dialog with FC_Disabled mode
    if (mode_ == FC_Enabled || (isVisible() && mode_ == FC_Disabled))
        setCheckboxesVisible(mode_ == FC_Enabled);

    if (mode_ == FC_Enabled)
        ui->rb_ignore->setChecked(true);

    // updateFilterDisplay();
}

void DialogDbCreation::showEvent(QShowEvent *event)
{
    //if (mode_ == FC_Hidden) // if the mode_ was set specifically by ::setFilterCreation(mode_ != FC_Hidden),
        updateViewMode(); // this function was already executed

    QDialog::showEvent(event);
}

void DialogDbCreation::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        activateItem(ui->treeWidget->currentItem());
        return;
    }

    if (event->key() == Qt::Key_Escape && mode_ == FC_Enabled) {
        if (itemsContain(TreeWidgetFileTypes::Checked))
            clearChecked();
        else
            setFilterCreation(FC_Disabled);
        return;
    }

    QDialog::keyPressEvent(event);
}
