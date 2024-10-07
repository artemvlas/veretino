#include "dialogdbcreation.h"
#include "ui_dialogdbcreation.h"
#include "tools.h"
#include <QPushButton>
#include <QDebug>
#include <QMenu>

const QMap<QString, QStringList> DialogDbCreation::_presets = {
    { QStringLiteral(u"Documents"), { QStringLiteral(u"odt"), QStringLiteral(u"ods"), QStringLiteral(u"pdf"), QStringLiteral(u"docx"),
                                      QStringLiteral(u"xlsx"), QStringLiteral(u"doc"), QStringLiteral(u"rtf"), QStringLiteral(u"txt"),
                                      QStringLiteral(u"epub"), QStringLiteral(u"fb2"), QStringLiteral(u"djvu") }},

    { QStringLiteral(u"Pictures"), { QStringLiteral(u"jpg"), QStringLiteral(u"jpeg"), QStringLiteral(u"png"), QStringLiteral(u"gif"),
                                     QStringLiteral(u"svg"), QStringLiteral(u"webp"), QStringLiteral(u"raw"), QStringLiteral(u"psd") }},

    { QStringLiteral(u"Music"), { QStringLiteral(u"flac"), QStringLiteral(u"wv"), QStringLiteral(u"ape"), QStringLiteral(u"oga"),
                                  QStringLiteral(u"ogg"), QStringLiteral(u"opus"), QStringLiteral(u"m4a"), QStringLiteral(u"mp3"),
                                  QStringLiteral(u"wav") }},

    { QStringLiteral(u"Videos"), { QStringLiteral(u"mkv"), QStringLiteral(u"webm"), QStringLiteral(u"mp4"),
                                   QStringLiteral(u"m4v"), QStringLiteral(u"avi") }},

    { QStringLiteral(u"! Triflings"), { QStringLiteral(u"log"), QStringLiteral(u"cue"), QStringLiteral(u"info"), QStringLiteral(u"txt") }}
};

DialogDbCreation::DialogDbCreation(const QString &folderPath, const FileTypeList &extList, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogDbCreation)
    , workDir_(folderPath)
{
    ui->setupUi(this);

    IconProvider _icons(palette());
    setWindowIcon(_icons.iconFolder());
    types_ = ui->treeWidget;
    types_->setColumnWidth(ItemFileType::ColumnType, 130);
    types_->setColumnWidth(ItemFileType::ColumnFilesNumber, 130);
    types_->sortByColumn(ItemFileType::ColumnTotalSize, Qt::DescendingOrder);

    ui->cb_top10->setVisible(extList.size() > 15);

    setTotalInfo(extList);
    connections();
    types_->setItems(extList);

    ui->tabWidget->setTabIcon(0, _icons.icon(Icons::Filter));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setIcon(_icons.icon(FileStatus::Calculating));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral(u"Continue"));

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

    connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &DialogDbCreation::resetView);

    connect(ui->cb_editable_exts, &QCheckBox::toggled, this, [=](bool _chk) { ui->le_exts_list->setReadOnly(!_chk); });
    connect(ui->le_exts_list, &QLineEdit::textEdited, this, &DialogDbCreation::parseInputedExts);

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
    setFilterConfig();

    if (settings_->filter_remember_exts)
        restoreLastExts();
}

void DialogDbCreation::updateSettings()
{
    if (!settings_)
        return;

    settings_->isLongExtension = ui->rb_ext_long->isChecked();
    settings_->addWorkDirToFilename = ui->cb_add_folder_name->isChecked();
    settings_->dbFlagConst = ui->cb_flag_const->isChecked();
    if (!ui->inp_db_filename->text().isEmpty())
        settings_->dbPrefix = format::simplifiedChars(ui->inp_db_filename->text());

    // filter
    settings_->filter_editable_exts = ui->cb_editable_exts->isChecked();
    settings_->filter_remember_exts = ui->cb_remember_exts->isChecked();

    if (mode_ == FC_Enabled) {
        settings_->filter_last_exts = types_->checkedExtensions();
        settings_->filter_mode = ui->rb_include->isChecked() ? FilterMode::Include : FilterMode::Ignore;
    }
    else {
        settings_->filter_last_exts = QStringList();
        settings_->filter_mode = FilterMode::NotSet;
    }
}

void DialogDbCreation::setDbConfig()
{
    if (!settings_)
        return;

    ui->rb_ext_short->setChecked(!settings_->isLongExtension);
    ui->cb_add_folder_name->setChecked(settings_->addWorkDirToFilename);
    ui->cb_flag_const->setChecked(settings_->dbFlagConst);

    if (!settings_->dbPrefix.isEmpty() && (settings_->dbPrefix != Lit::s_db_prefix))
        ui->inp_db_filename->setText(settings_->dbPrefix);

    updateLabelDbFilename();
}

void DialogDbCreation::setFilterConfig()
{
    if (!settings_)
        return;

    ui->cb_editable_exts->setChecked(settings_->filter_editable_exts);
    ui->cb_remember_exts->setChecked(settings_->filter_remember_exts);
}

void DialogDbCreation::restoreLastExts()
{
    if (!settings_
        || settings_->filter_last_exts.isEmpty()
        || settings_->filter_mode == FilterMode::NotSet)
    {
        return;
    }

    if (mode_ != FC_Enabled) {
        setFilterCreation(FC_Enabled);
    }

    types_->setChecked(settings_->filter_last_exts);

    if (settings_->filter_mode == FilterMode::Ignore)
        ui->rb_ignore->setChecked(true);
    else if (settings_->filter_mode == FilterMode::Include)
        ui->rb_include->setChecked(true);
}

void DialogDbCreation::parseInputedExts()
{
    if (ui->le_exts_list->isReadOnly())
        return;

    const QStringList _exts = extensionsList();
    types_->setChecked(_exts);
}

QStringList DialogDbCreation::extensionsList() const
{
    if (ui->le_exts_list->text().isEmpty())
        return QStringList();

    QString _inputed = ui->le_exts_list->text().toLower();
    _inputed.remove('*');
    _inputed.replace(" ."," ");
    _inputed.replace(' ',',');

    if (_inputed.startsWith('.'))
        _inputed.remove(0, 1);

    QStringList _exts = _inputed.split(',', Qt::SkipEmptyParts);
    _exts.removeDuplicates();
    return _exts;
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

void DialogDbCreation::setTotalInfo(const FileTypeList &exts)
{
    ui->l_total_files->setText(QString("Total: %1 types, %2 ")
                                .arg(exts.size())
                                .arg(format::filesNumSize(Files::totalListed(exts))));
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
        ui->l_total_filtered->setText(QStringLiteral(u"No filter"));
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
    ui->cb_enable_filter->setChecked(mode_ == FC_Enabled);

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

    QMap<QString, QStringList>::const_iterator it;
    for (it = _presets.constBegin(); it != _presets.constEnd(); ++it) {
        QAction *_act = new QAction(it.key(), dispMenu);
        dispMenu->addAction(_act);
    }

    dispMenu->exec(types_->mapToGlobal(point));
}

void DialogDbCreation::handlePresetClicked(const QAction *_act)
{
    if (mode_ != FC_Enabled) {
        setFilterCreation(FC_Enabled);
    }

    types_->setChecked(_presets.value(_act->text()));

    QRadioButton *_rb = _act->text().startsWith('!') ? ui->rb_ignore
                                                     : ui->rb_include;
    _rb->setChecked(true);
}

void DialogDbCreation::resetView()
{
    clearChecked();
    ui->inp_db_filename->clear();
    ui->rb_ext_long->setChecked(true);
    ui->cb_add_folder_name->setChecked(true);
    ui->cb_flag_const->setChecked(false);
}

void DialogDbCreation::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        && types_->hasFocus())
    {
        activateItem(types_->currentItem());
        return;
    }

    if (event->key() == Qt::Key_Escape
        && mode_ == FC_Enabled
        && ui->tabWidget->currentIndex() == 0)
    {
        if (itemsContain(WidgetFileTypes::Checked))
            clearChecked();
        else
            setFilterCreation(FC_Disabled);
        return;
    }

    QDialog::keyPressEvent(event);
}
