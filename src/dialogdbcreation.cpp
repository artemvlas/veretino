#include "dialogdbcreation.h"
#include "ui_dialogdbcreation.h"
#include "tools.h"
#include <QPushButton>
#include <QDebug>
#include <QMenu>

const QMap<QString, QSet<QString>> DialogDbCreation::_presets = {
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

    ui->cb_flag_const->setToolTip(QStringLiteral(u"The database will contain a flag\n"
                                                 "to prevent changes from being made."));

    updateViewMode();
}

DialogDbCreation::~DialogDbCreation()
{
    delete ui;
}

void DialogDbCreation::connections()
{
    // type list
    connect(types_, &WidgetFileTypes::customContextMenuRequested, this, &DialogDbCreation::createMenuWidgetTypes);
    connect(types_, &QTreeWidget::itemChanged, this, &DialogDbCreation::updateFilterDisplay);
    connect(types_, &QTreeWidget::itemDoubleClicked, this, &DialogDbCreation::activateItem);
    connect(ui->cb_top10, &QCheckBox::toggled, this, &DialogDbCreation::setItemsVisibility);

    // filter
    connect(ui->cb_enable_filter, &QCheckBox::toggled, this,
            [=](bool isChecked){ setFilterCreation(isChecked ? FC_Enabled : FC_Disabled); });
    connect(ui->rb_ignore, &QRadioButton::toggled, this, &DialogDbCreation::updateFilterDisplay);
    connect(ui->cb_editable_exts, &QCheckBox::toggled, this, [=](bool _chk) { ui->le_exts_list->setReadOnly(!_chk); });
    connect(ui->le_exts_list, &QLineEdit::textEdited, this, &DialogDbCreation::parseInputedExts);

    // filename
    connect(ui->rb_ext_short, &QRadioButton::toggled, this, &DialogDbCreation::updateDbFilename);
    connect(ui->cb_add_folder_name, &QCheckBox::toggled, this, &DialogDbCreation::updateDbFilename);
    connect(ui->inp_db_filename, &QLineEdit::textEdited, this, &DialogDbCreation::updateDbFilename);

    // settings
    connect(this, &DialogDbCreation::accepted, this, &DialogDbCreation::updateSettings);

    // buttons
    connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &DialogDbCreation::resetView);
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

    const QString _inpFileName = ui->inp_db_filename->text();
    if (!_inpFileName.isEmpty())
        settings_->dbPrefix = format::simplifiedChars(_inpFileName);

    settings_->isLongExtension = ui->rb_ext_long->isChecked();
    settings_->addWorkDirToFilename = ui->cb_add_folder_name->isChecked();
    settings_->dbFlagConst = ui->cb_flag_const->isChecked();

    // filter
    settings_->filter_editable_exts = ui->cb_editable_exts->isChecked();
    settings_->filter_remember_exts = ui->cb_remember_exts->isChecked();

    settings_->filter_mode = curFilterMode();
    settings_->filter_last_exts = (mode_ == FC_Enabled) ? types_->checkedExtensions()
                                                        : QStringList();

    // algo
    settings_->setAlgorithm(tools::strToAlgo(ui->cmb_algo->currentText()));
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

    updateDbFilename();

    // algo
    ui->cmb_algo->addItems(Lit::sl_digest_Exts);
    ui->cmb_algo->setCurrentIndex(cmbAlgoIndex());
}

int DialogDbCreation::cmbAlgoIndex()
{
    if (!settings_)
        return 0;

    switch (settings_->algorithm()) {
    case QCryptographicHash::Sha1:
        return 0;
    case QCryptographicHash::Sha256:
        return 1;
    case QCryptographicHash::Sha512:
        return 2;
    default:
        return 1;
    }
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

    QRadioButton *_rb = (settings_->filter_mode == FilterMode::Include) ? ui->rb_include
                                                                        : ui->rb_ignore;
    _rb->setChecked(true);
}

void DialogDbCreation::parseInputedExts()
{
    if (ui->le_exts_list->isReadOnly())
        return;

    types_->setChecked(inputedExts());
}

QStringList DialogDbCreation::inputedExts() const
{
    QString _inputed = ui->le_exts_list->text().toLower();

    if (_inputed.isEmpty())
        return QStringList();

    _inputed.remove('*');
    _inputed.replace(" ."," ");
    _inputed.replace(' ',',');

    if (_inputed.startsWith('.'))
        _inputed.remove(0, 1);

    return _inputed.split(',', Qt::SkipEmptyParts);
}

FilterMode DialogDbCreation::curFilterMode() const
{
    if (mode_ == FC_Disabled)
        return FilterMode::NotSet;

    return ui->rb_include->isChecked() ? FilterMode::Include : FilterMode::Ignore;
}

void DialogDbCreation::updateDbFilename()
{
    const QString _inpText = ui->inp_db_filename->text();
    const QString _prefix = _inpText.isEmpty() ? Lit::s_db_prefix : format::simplifiedChars(_inpText);
    const QString &_folderName = ui->cb_add_folder_name->isChecked() ? workDir_ : QString(); // QStringLiteral(u"@FolderName")
    const QString _ext = Lit::sl_db_exts.at(ui->rb_ext_short->isChecked());
    const QString _dbFileName = format::composeDbFileName(_prefix, _folderName, _ext);
    const QString _dbFilePath = paths::joinPath(workDir_, _dbFileName);
    const bool _exists = QFileInfo::exists(_dbFilePath);
    const QString _color = _exists ? format::coloredText(true) : QString();
    const QString _toolTip = _exists ? QStringLiteral(u"The file already exists") : QStringLiteral(u"Available");
    const QString _acceptBtnText = _exists ? QStringLiteral(u"Overwrite") : QStringLiteral(u"Create");

    ui->l_db_filename->setStyleSheet(_color);
    ui->l_db_filename->setText(_dbFileName);
    ui->l_db_filename->setToolTip(_toolTip);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(_acceptBtnText);
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

void DialogDbCreation::updateFilterDisplay()
{
    updateLabelFilterExtensions();
    updateLabelTotalFiltered();

    // TMP
    bool isFiltered = ui->rb_include->isChecked() ? types_->hasChecked()
                                                  : types_->hasChecked()
                                                        && types_->itemsContain(WidgetFileTypes::UnChecked);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled((mode_ != FC_Enabled)
                                                            || !types_->hasChecked()
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

    QString __s;
    if (types_->hasChecked()) {
        __s = QStringLiteral(u"Filtered: ") + format::filesNumSize(types_->numSize(_checkState));
    }
    else {
        __s = QStringLiteral(u"No filter");
    }

    ui->l_total_filtered->setText(__s);
}

FilterRule DialogDbCreation::resultFilter()
{
    return FilterRule(curFilterMode(),
                      types_->checkedExtensions());
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

    QMap<QString, QSet<QString>>::const_iterator it;
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
    ui->cmb_algo->setCurrentIndex(cmbAlgoIndex());
    updateDbFilename();
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
        if (types_->hasChecked())
            clearChecked();
        else
            setFilterCreation(FC_Disabled);
        return;
    }

    QDialog::keyPressEvent(event);
}
