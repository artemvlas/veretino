#include "dialogdbcreation.h"
#include "ui_dialogdbcreation.h"
#include "tools.h"
#include <QPushButton>
#include <QDebug>
#include <QMenu>

DialogDbCreation::DialogDbCreation(const QString &folderPath, const QList<ExtNumSize> &extList, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogDbCreation)
    , workDir_(folderPath)
{
    ui->setupUi(this);

    icons_.setTheme(palette());
    setWindowIcon(icons_.iconFolder());
    types_ = ui->treeWidget;
    types_->setColumnWidth(ItemFileType::ColumnType, 130);
    types_->setColumnWidth(ItemFileType::ColumnFilesNumber, 130);
    types_->sortByColumn(ItemFileType::ColumnTotalSize, Qt::DescendingOrder);

    //QString folderName = paths::shortenPath(folderPath);
    //ui->labelFolderName->setText(folderName);
    //ui->labelFolderName->setToolTip(folderPath);
    ui->cb_top10->setVisible(extList.size() > 15);

    setTotalInfo(extList);
    connections();
    types_->setItems(extList);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setIcon(icons_.icon(FileStatus::Calculating));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Continue");

    updateViewMode();
}

DialogDbCreation::~DialogDbCreation()
{
    delete ui;
}

void DialogDbCreation::connections()
{
    connect(types_, &WidgetFileTypes::customContextMenuRequested, this, &DialogDbCreation::createMenuWidgetTypes);

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
    settings_->dbPrefix = ui->inp_db_filename->text().isEmpty() ? Lit::s_db_prefix
                                                                : format::simplifiedChars(ui->inp_db_filename->text());
}

void DialogDbCreation::setDbConfig()
{
    if (!settings_)
        return;

    ui->rb_ext_short->setChecked(!settings_->isLongExtension);
    ui->cb_add_folder_name->setChecked(settings_->addWorkDirToFilename);
    ui->cb_flag_const->setChecked(settings_->dbFlagConst);

    if (settings_->dbPrefix != Lit::s_db_prefix)
        ui->inp_db_filename->setText(settings_->dbPrefix);

    updateLabelDbFilename();
}

void DialogDbCreation::updateLabelDbFilename()
{
    QString prefix = ui->inp_db_filename->text().isEmpty() ? Lit::s_db_prefix : format::simplifiedChars(ui->inp_db_filename->text());
    QString folderName = ui->cb_add_folder_name->isChecked() ? QStringLiteral(u"@FolderName") : QString();
    QString extension = Lit::sl_db_exts.at(ui->rb_ext_short->isChecked());

    ui->l_db_filename->setText(format::composeDbFileName(prefix, folderName, extension));
}

void DialogDbCreation::setItemsVisibility(bool isTop10Checked)
{
    if (!isTop10Checked) {
        types_->showAllItems();
        ui->cb_top10->setText(QStringLiteral(u"Top10"));
    }
    else {
        types_->hideExtra();
        ui->cb_top10->setText(QStringLiteral(u"Top10: ")
                              + format::filesNumSize(types_->numSizeVisible()));
    }

    updateFilterDisplay();
}

void DialogDbCreation::setTotalInfo(const QList<ExtNumSize> &exts)
{
    NumSize _nums;

    for (const ExtNumSize &_ext : exts) {
        _nums.add(_ext.filesNumber, _ext.filesSize);
    }

    ui->l_total_files->setText(QString("Total: %1 types, %2 ")
                                .arg(exts.size())
                                .arg(format::filesNumSize(_nums)));
}

void DialogDbCreation::setCheckboxesVisible(bool visible)
{
    types_->setCheckboxesVisible(visible);
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
    if (mode_ == FC_Disabled) {
        setFilterCreation(FC_Enabled);
        return;
    }

    ItemFileType *item = static_cast<ItemFileType*>(t_item);
    item->toggle();
}


bool DialogDbCreation::itemsContain(int state) const
{
    if (mode_ != FC_Enabled)
        return false;

    return types_->itemsContain((WidgetFileTypes::CheckState)state);
}

void DialogDbCreation::updateFilterDisplay()
{
    updateLabelFilterExtensions();
    updateLabelTotalFiltered();

    // TMP
    bool isFiltered = ui->rb_include->isChecked() ? types_->itemsContain(WidgetFileTypes::Checked)
                                                  : types_->itemsContain(WidgetFileTypes::Checked)
                                                        && types_->itemsContain(WidgetFileTypes::UnChecked);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled((mode_ != FC_Enabled)
                                                            || !types_->itemsContain(WidgetFileTypes::Checked)
                                                            || isFiltered);

    // !!! TMP !!!
}

void DialogDbCreation::updateLabelFilterExtensions()
{
    if (mode_ != FC_Enabled) {
        ui->le_exts_list->clear();
        return;
    }

    const QString  _color = format::coloredText(QStringLiteral(u"QLineEdit"), ui->rb_ignore->isChecked());
    ui->le_exts_list->setStyleSheet(_color);
    ui->le_exts_list->setText(types_->checkedExtensions().join(QStringLiteral(u", ")));
}

void DialogDbCreation::updateLabelTotalFiltered()
{
    if (mode_ != FC_Enabled) {
        ui->l_total_filtered->clear();
        return;
    }

    // ? Include only visible_checked : Include all except visible_checked and Db-Sha
    WidgetFileTypes::CheckState _checkState = ui->rb_include->isChecked() ? WidgetFileTypes::Checked
                                                                          : WidgetFileTypes::UnChecked;

    if (itemsContain(WidgetFileTypes::Checked)) {
        ui->l_total_filtered->setText(QStringLiteral(u"Filtered: ")
                                        + format::filesNumSize(types_->numSize(_checkState)));
    }
    else {
        //ui->l_total_filtered->clear();
        ui->l_total_filtered->setText("No filter");
    }
}

FilterRule DialogDbCreation::resultFilter()
{
    FilterRule::FilterMode __f = ui->rb_ignore->isChecked() ? FilterRule::Ignore : FilterRule::Include;
    return FilterRule(__f, types_->checkedExtensions());
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

void DialogDbCreation::createMenuWidgetTypes(const QPoint &point)
{
    QMenu *dispMenu = new QMenu(this);
    connect(dispMenu, &QMenu::aboutToHide, dispMenu, &QMenu::deleteLater);
    connect(dispMenu, &QMenu::triggered, this, &DialogDbCreation::handlePresetClicked);

    for (int i = 0; i < filterPresetsList.size(); ++i) {
        QAction *action = new QAction(filterPresetsList.at(i), dispMenu);
        dispMenu->addAction(action);
    }

    dispMenu->exec(types_->mapToGlobal(point));
}

void DialogDbCreation::handlePresetClicked(const QAction *_act)
{
    // TMP !!!
    if (mode_ != FC_Enabled) {
        setFilterCreation(FC_Enabled);
    }

    clearChecked();

    const int _ind = filterPresetsList.indexOf(_act->text());

    switch (_ind) {
    case 0:
        types_->setChecked(listPresetDocuments);
        break;
    case 1:
        types_->setChecked(listPresetPictures);
        break;
    case 2:
        types_->setChecked(listPresetMusic);
        break;
    case 3:
        types_->setChecked(listPresetVideos);
        break;
    case 4:
        types_->setChecked(listPresetIgnoreTriflings);
        break;
    default: break;
    }

    if (_ind == 4)
        ui->rb_ignore->setChecked(true);
    else if (_ind >= 0 && _ind < 4)
        ui->rb_include->setChecked(true);

    // TMP !!!
}

void DialogDbCreation::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        activateItem(types_->currentItem());
        return;
    }

    if (event->key() == Qt::Key_Escape && mode_ == FC_Enabled) {
        if (itemsContain(WidgetFileTypes::Checked))
            clearChecked();
        else
            setFilterCreation(FC_Disabled);
        return;
    }

    QDialog::keyPressEvent(event);
}

// moved from Settings
/*
enum FilterPreset { PresetCustom, PresetDocuments, PresetPictures, PresetMusic, PresetVideos, PresetIgnoreTriflings };

QStringList DialogSettings::extensionsList() const
{
    if (ui->inputExtensions->text().isEmpty())
        return QStringList();

    QString _inputed = ui->inputExtensions->text().toLower();
    _inputed.remove('*');
    _inputed.replace(" ."," ");
    _inputed.replace(' ',',');

    if (_inputed.startsWith('.'))
        _inputed.remove(0, 1);

    QStringList ext = _inputed.split(',', Qt::SkipEmptyParts);
    ext.removeDuplicates();
    return ext;
}

void DialogSettings::cleanUpExtList()
{
    QStringList _exts = extensionsList();
    if (!_exts.isEmpty()) {
        QStringList list;
        if (!ui->rbInclude->isChecked()) {
            if (ui->ignoreShaFiles->isChecked())
                list.append(Lit::sl_digest_exts); // { "sha1", "sha256", "sha512" }
            if (ui->ignoreDbFiles->isChecked())
                list.append(Lit::sl_db_exts); // { "ver.json", "ver" }
        }

        if (!list.isEmpty()) {
            foreach (const QString &str, list) {
                _exts.removeOne(str);
            }
            ui->inputExtensions->setText(_exts.join(','));
        }
    }
}

*/
