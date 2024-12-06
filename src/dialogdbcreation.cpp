/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogdbcreation.h"
#include "ui_dialogdbcreation.h"
#include "tools.h"
#include "pathstr.h"
#include <QPushButton>
#include <QDebug>
#include <QMenu>

const QMap<QString, QSet<QString>> DialogDbCreation::_presets = {
    { QStringLiteral(u"Archives"), { QStringLiteral(u"zip"), QStringLiteral(u"7z"), QStringLiteral(u"gz"), QStringLiteral(u"tgz"),
                                   QStringLiteral(u"tar"), QStringLiteral(u"rar"), QStringLiteral(u"img"), QStringLiteral(u"iso") }},

    { QStringLiteral(u"Documents"), { QStringLiteral(u"odt"), QStringLiteral(u"ods"), QStringLiteral(u"pdf"), QStringLiteral(u"docx"),
                                      QStringLiteral(u"xlsx"), QStringLiteral(u"doc"), QStringLiteral(u"rtf"), QStringLiteral(u"txt"),
                                      QStringLiteral(u"epub"), QStringLiteral(u"fb2"), QStringLiteral(u"djvu") }},

    { QStringLiteral(u"Pictures"), { QStringLiteral(u"jpg"), QStringLiteral(u"jpeg"), QStringLiteral(u"png"), QStringLiteral(u"gif"),
                                     QStringLiteral(u"svg"), QStringLiteral(u"webp"), QStringLiteral(u"raw"), QStringLiteral(u"psd") }},

    { QStringLiteral(u"Music"), { QStringLiteral(u"flac"), QStringLiteral(u"wv"), QStringLiteral(u"ape"), QStringLiteral(u"oga"),
                                  QStringLiteral(u"ogg"), QStringLiteral(u"opus"), QStringLiteral(u"m4a"), QStringLiteral(u"mp3"),
                                  QStringLiteral(u"wav") }},

    { QStringLiteral(u"Videos"), { QStringLiteral(u"mkv"), QStringLiteral(u"webm"), QStringLiteral(u"mp4"), QStringLiteral(u"m4v"),
                                   QStringLiteral(u"avi"), QStringLiteral(u"mpg"), QStringLiteral(u"mov") }},

    { QStringLiteral(u"! Triflings"), { QStringLiteral(u"log"), QStringLiteral(u"cue"), QStringLiteral(u"info"), QStringLiteral(u"inf"),
                                        QStringLiteral(u"srt"), QStringLiteral(u"m3u"), QStringLiteral(u"bak"), QStringLiteral(u"bat"),
                                        QStringLiteral(u"sh"), QStringLiteral(u"txt") }}
};

DialogDbCreation::DialogDbCreation(const QString &folderPath, const FileTypeList &extList, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogDbCreation)
    , workDir_(folderPath)
{
    ui->setupUi(this);

    _icons.setTheme(palette());
    setWindowIcon(_icons.icon(Icons::Database));
    types_ = ui->treeWidget;
    types_->setColumnWidth(ItemFileType::ColumnType, 130);
    types_->setColumnWidth(ItemFileType::ColumnFilesNumber, 130);
    types_->sortByColumn(ItemFileType::ColumnTotalSize, Qt::DescendingOrder);

    ui->cb_top10->setVisible(extList.size() > 15);

    setTotalInfo(extList);
    connections();
    types_->setItems(extList);

    ui->tabWidget->tabBar()->setTabButton(0, QTabBar::LeftSide, cb_file_filter);
    ui->tabWidget->setTabIcon(0, _icons.icon(Icons::Filter));
    ui->tabWidget->setTabIcon(1, _icons.icon(Icons::Configure));
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
    connect(cb_file_filter, &QCheckBox::toggled, this,
            [=](bool isChecked){ setFilterCreation(isChecked ? FC_Enabled : FC_Disabled); });
    connect(ui->rb_ignore, &QRadioButton::toggled, this, &DialogDbCreation::updateFilterDisplay);
    connect(ui->cb_editable_exts, &QCheckBox::toggled, this, [=](bool _chk) { ui->le_exts_list->setReadOnly(!_chk); });
    connect(ui->le_exts_list, &LineEdit::edited, this, &DialogDbCreation::parseInputedExts);

    // filename
    connect(ui->rb_ext_short, &QRadioButton::toggled, this, &DialogDbCreation::updateDbFilename);
    connect(ui->cb_add_folder_name, &QCheckBox::toggled, this, &DialogDbCreation::updateDbFilename);
    connect(ui->inp_db_prefix, &QLineEdit::textEdited, this, &DialogDbCreation::updateDbFilename);

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

    const QString _inpPrefix = ui->inp_db_prefix->text();
    settings_->dbPrefix = (_inpPrefix != Lit::s_db_prefix) ? format::simplifiedChars(_inpPrefix)
                                                           : QString();

    settings_->isLongExtension = ui->rb_ext_long->isChecked();
    settings_->addWorkDirToFilename = ui->cb_add_folder_name->isChecked();
    settings_->dbFlagConst = ui->cb_flag_const->isChecked();

    // filter
    settings_->filter_editable_exts = ui->cb_editable_exts->isChecked();
    settings_->filter_remember_exts = ui->cb_remember_exts->isChecked();

    settings_->filter_mode = curFilterMode();
    settings_->filter_last_exts = isFilterCreating() ? types_->checkedExtensions()
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
        ui->inp_db_prefix->setText(settings_->dbPrefix);

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

    if (!isFilterCreating()) {
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
    _inputed.replace(QStringLiteral(u" ."), QStringLiteral(u","));
    _inputed.replace(' ',',');

    if (_inputed.startsWith('.'))
        _inputed.remove(0, 1);

    return _inputed.split(',', Qt::SkipEmptyParts);
}

FilterMode DialogDbCreation::curFilterMode() const
{
    if (!isFilterCreating())
        return FilterMode::NotSet;

    return ui->rb_include->isChecked() ? FilterMode::Include : FilterMode::Ignore;
}

void DialogDbCreation::updateDbFilename()
{
    const QString _inpText = ui->inp_db_prefix->text();
    const QString _prefix = _inpText.isEmpty() ? Lit::s_db_prefix : format::simplifiedChars(_inpText);
    const QString &_folderName = ui->cb_add_folder_name->isChecked() ? workDir_ : QString(); // QStringLiteral(u"@FolderName")
    const QString _ext = Lit::sl_db_exts.at(ui->rb_ext_short->isChecked());
    const QString _dbFileName = format::composeDbFileName(_prefix, _folderName, _ext);
    const QString _dbFilePath = pathstr::joinPath(workDir_, _dbFileName);
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
    QString __s;

    if (isTop10Checked) {
        types_->hideExtra();
        __s = QStringLiteral(u"Top10: ") + format::filesNumSize(types_->numSizeVisible());
    }
    else {
        types_->showAllItems();
        __s = QStringLiteral(u"Top10");
    }

    ui->cb_top10->setText(__s);
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
}

void DialogDbCreation::clearChecked()
{
    if (isFilterCreating()) {
        ui->rb_ignore->setChecked(true);
        setCheckboxesVisible(true);
    }
}

void DialogDbCreation::activateItem(QTreeWidgetItem *t_item)
{
    if (!isFilterCreating()) {
        setFilterCreation(FC_Enabled);
        return;
    }

    ItemFileType *_item = static_cast<ItemFileType*>(t_item);
    _item->toggle();
}

void DialogDbCreation::updateFilterDisplay()
{
    updateLabelFilterExtensions();
    updateLabelTotalFiltered();

    // TMP
    const bool isFiltered = ui->rb_include->isChecked() ? types_->hasChecked()
                                                        : types_->hasChecked()
                                                              && types_->itemsContain(WidgetFileTypes::UnChecked);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled((!isFilterCreating())
                                                            || !types_->hasChecked()
                                                            || isFiltered);

    // !!! TMP !!!
}

void DialogDbCreation::updateLabelFilterExtensions()
{
    if (!isFilterCreating()) {
        ui->le_exts_list->clear();
        return;
    }

    const QString  _color = format::coloredText(QStringLiteral(u"QLineEdit"), ui->rb_ignore->isChecked());
    ui->le_exts_list->setStyleSheet(_color);
    ui->le_exts_list->setText(types_->checkedExtensions().join(Lit::s_sepCommaSpace));
}

void DialogDbCreation::updateLabelTotalFiltered()
{
    if (!isFilterCreating()) {
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
    const bool _is_f = isFilterCreating();

    ui->fr_total_filtered->setVisible(_is_f);
    ui->fr_filter_exts->setVisible(_is_f);
    cb_file_filter->setChecked(_is_f);
    setCheckboxesVisible(_is_f);

    if (_is_f)
        ui->rb_ignore->setChecked(true);
}

QIcon DialogDbCreation::presetIcon(const QString &_name) const
{
    if (_name.startsWith('!'))
        return _icons.icon(Icons::ClearHistory);

    QString _ext;
    if (!_name.compare(QStringLiteral(u"Archives")))
        _ext = QStringLiteral(u"zip");
    else if (!_name.compare(QStringLiteral(u"Documents")))
        _ext = QStringLiteral(u"docx");
    else if (!_name.compare(QStringLiteral(u"Pictures")))
        _ext = QStringLiteral(u"jpg");
    else if (!_name.compare(QStringLiteral(u"Music")))
        _ext = QStringLiteral(u"mp3");
    else if (!_name.compare(QStringLiteral(u"Videos")))
        _ext = QStringLiteral(u"mkv");
    else
        return QIcon();

    return _icons.type_icon(_ext);
}

void DialogDbCreation::createMenuWidgetTypes(const QPoint &point)
{
    QMenu *dispMenu = new QMenu(this);
    connect(dispMenu, &QMenu::aboutToHide, dispMenu, &QMenu::deleteLater);
    connect(dispMenu, &QMenu::triggered, this, &DialogDbCreation::handlePresetClicked);

    QMap<QString, QSet<QString>>::const_iterator it;
    for (it = _presets.constBegin(); it != _presets.constEnd(); ++it) {
        QAction *_act = new QAction(presetIcon(it.key()), it.key(), dispMenu);
        dispMenu->addAction(_act);
    }

    dispMenu->exec(types_->mapToGlobal(point));
}

void DialogDbCreation::handlePresetClicked(const QAction *_act)
{
    if (!isFilterCreating()) {
        setFilterCreation(FC_Enabled);
    }

    // _act->text() may contain an ampersand in some Qt versions (6.7.2 for example)
    const QString __s = _act->toolTip();
    types_->setChecked(_presets.value(__s));

    QRadioButton *_rb = __s.startsWith('!') ? ui->rb_ignore
                                            : ui->rb_include;
    _rb->setChecked(true);
}

void DialogDbCreation::resetView()
{
    clearChecked();
    ui->inp_db_prefix->clear();
    ui->rb_ext_long->setChecked(true);
    ui->cb_add_folder_name->setChecked(true);
    ui->cb_flag_const->setChecked(false);
    ui->cmb_algo->setCurrentIndex(cmbAlgoIndex());
    updateDbFilename();
}

bool DialogDbCreation::isFilterCreating() const
{
    return (mode_ == FC_Enabled);
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
        && isFilterCreating()
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
