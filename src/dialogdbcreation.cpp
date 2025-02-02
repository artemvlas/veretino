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

const QMap<QString, QSet<QString>> DialogDbCreation::s_presets = {
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
                                        QStringLiteral(u"m3u"), QStringLiteral(u"bak"), QStringLiteral(u"bat"), QStringLiteral(u"sh") }}
};

DialogDbCreation::DialogDbCreation(const QString &folderPath, const FileTypeList &extList, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogDbCreation)
    , m_workDir(folderPath)
{
    ui->setupUi(this);

    m_icons.setTheme(palette());
    setWindowIcon(m_icons.icon(Icons::Database));
    m_types = ui->treeWidget;
    m_types->setColumnWidth(ItemFileType::ColumnType, 130);
    m_types->setColumnWidth(ItemFileType::ColumnFilesNumber, 130);
    m_types->sortByColumn(ItemFileType::ColumnTotalSize, Qt::DescendingOrder);

    ui->cb_top10->setVisible(extList.count() > 15);

    setTotalInfo(extList);
    connections();
    m_types->setItems(extList);

    ui->tabWidget->tabBar()->setTabButton(0, QTabBar::LeftSide, cb_file_filter);
    ui->tabWidget->setTabIcon(0, m_icons.icon(Icons::Filter));
    ui->tabWidget->setTabIcon(1, m_icons.icon(Icons::Configure));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setIcon(m_icons.icon(FileStatus::Calculating));

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
    connect(m_types, &WidgetFileTypes::customContextMenuRequested, this, &DialogDbCreation::createMenuWidgetTypes);
    connect(m_types, &QTreeWidget::itemChanged, this, &DialogDbCreation::updateFilterDisplay);
    connect(m_types, &QTreeWidget::itemDoubleClicked, this, &DialogDbCreation::activateItem);
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
    m_settings = settings;
    setDbConfig();
    setFilterConfig();

    if (m_settings->filter_remember_exts)
        restoreLastExts();
}

void DialogDbCreation::updateSettings()
{
    if (!m_settings)
        return;

    const QString _inpPrefix = ui->inp_db_prefix->text();
    m_settings->dbPrefix = (_inpPrefix != Lit::s_db_prefix) ? format::simplifiedChars(_inpPrefix)
                                                           : QString();

    m_settings->isLongExtension = ui->rb_ext_long->isChecked();
    m_settings->addWorkDirToFilename = ui->cb_add_folder_name->isChecked();
    m_settings->dbFlagConst = ui->cb_flag_const->isChecked();

    // filter
    m_settings->filter_editable_exts = ui->cb_editable_exts->isChecked();
    m_settings->filter_remember_exts = ui->cb_remember_exts->isChecked();

    m_settings->filter_mode = curFilterMode();
    m_settings->filter_last_exts = isFilterCreating() ? m_types->checkedExtensions()
                                                      : QStringList();

    // algo
    m_settings->setAlgorithm(tools::strToAlgo(ui->cmb_algo->currentText()));
}

void DialogDbCreation::setDbConfig()
{
    if (!m_settings)
        return;

    ui->rb_ext_short->setChecked(!m_settings->isLongExtension);
    ui->cb_add_folder_name->setChecked(m_settings->addWorkDirToFilename);
    ui->cb_flag_const->setChecked(m_settings->dbFlagConst);

    if (!m_settings->dbPrefix.isEmpty() && (m_settings->dbPrefix != Lit::s_db_prefix))
        ui->inp_db_prefix->setText(m_settings->dbPrefix);

    updateDbFilename();

    // algo
    ui->cmb_algo->addItems(Lit::sl_digest_Exts.mid(1)); // all except index 0 (MD5)
    ui->cmb_algo->setCurrentIndex(cmbAlgoIndex());
}

int DialogDbCreation::cmbAlgoIndex()
{
    if (!m_settings)
        return 0;

    switch (m_settings->algorithm()) {
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
    if (!m_settings)
        return;

    ui->cb_editable_exts->setChecked(m_settings->filter_editable_exts);
    ui->cb_remember_exts->setChecked(m_settings->filter_remember_exts);
}

void DialogDbCreation::restoreLastExts()
{
    if (!m_settings
        || m_settings->filter_last_exts.isEmpty()
        || m_settings->filter_mode == FilterMode::NotSet)
    {
        return;
    }

    if (!isFilterCreating()) {
        setFilterCreation(FC_Enabled);
    }

    m_types->setChecked(m_settings->filter_last_exts);

    QRadioButton *p_rb = (m_settings->filter_mode == FilterMode::Include) ? ui->rb_include
                                                                          : ui->rb_ignore;
    p_rb->setChecked(true);
}

void DialogDbCreation::parseInputedExts()
{
    if (ui->le_exts_list->isReadOnly())
        return;

    m_types->setChecked(inputedExts());
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
    const QString &_folderName = ui->cb_add_folder_name->isChecked() ? m_workDir : QString(); // QStringLiteral(u"@FolderName")
    const QString _ext = Lit::sl_db_exts.at(ui->rb_ext_short->isChecked());
    const QString _dbFileName = format::composeDbFileName(_prefix, _folderName, _ext);
    const QString _dbFilePath = pathstr::joinPath(m_workDir, _dbFileName);
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
        m_types->hideExtra();
        __s = QStringLiteral(u"Top10: ") + format::filesNumSize(m_types->numSizeVisible());
    }
    else {
        m_types->showAllItems();
        __s = QStringLiteral(u"Top10");
    }

    ui->cb_top10->setText(__s);
    updateFilterDisplay();
}

void DialogDbCreation::setTotalInfo(const FileTypeList &exts)
{
    ui->l_total_files->setText(QString("Total: %1 types, %2 ")
                                .arg(exts.count())
                                .arg(format::filesNumSize(Files::totalListed(exts))));
}

void DialogDbCreation::setCheckboxesVisible(bool visible)
{
    m_types->setCheckboxesVisible(visible);
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
    const bool isFiltered = ui->rb_include->isChecked() ? m_types->hasChecked()
                                                        : m_types->hasChecked()
                                                              && m_types->itemsContain(WidgetFileTypes::UnChecked);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled((!isFilterCreating())
                                                            || !m_types->hasChecked()
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
    ui->le_exts_list->setText(m_types->checkedExtensions().join(Lit::s_sepCommaSpace));
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
    if (m_types->hasChecked()) {
        __s = QStringLiteral(u"Filtered: ") + format::filesNumSize(m_types->numSize(_checkState));
    }
    else {
        __s = QStringLiteral(u"No filter");
    }

    ui->l_total_filtered->setText(__s);
}

FilterRule DialogDbCreation::resultFilter()
{
    return FilterRule(curFilterMode(),
                      m_types->checkedExtensions());
}

void DialogDbCreation::setFilterCreation(FilterCreation mode)
{
    if (m_mode != mode) {
        m_mode = mode;
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

QIcon DialogDbCreation::presetIcon(const QString &name) const
{
    if (name.startsWith('!'))
        return m_icons.icon(Icons::ClearHistory);

    QString ext;
    if (!name.compare(QStringLiteral(u"Archives")))
        ext = QStringLiteral(u"zip");
    else if (!name.compare(QStringLiteral(u"Documents")))
        ext = QStringLiteral(u"docx");
    else if (!name.compare(QStringLiteral(u"Pictures")))
        ext = QStringLiteral(u"jpg");
    else if (!name.compare(QStringLiteral(u"Music")))
        ext = QStringLiteral(u"mp3");
    else if (!name.compare(QStringLiteral(u"Videos")))
        ext = QStringLiteral(u"mkv");
    else
        return QIcon();

    return m_icons.type_icon(ext);
}

void DialogDbCreation::createMenuWidgetTypes(const QPoint &point)
{
    QMenu *dispMenu = new QMenu(this);
    connect(dispMenu, &QMenu::aboutToHide, dispMenu, &QMenu::deleteLater);
    connect(dispMenu, &QMenu::triggered, this, &DialogDbCreation::handlePresetClicked);

    QMap<QString, QSet<QString>>::const_iterator it;
    for (it = s_presets.constBegin(); it != s_presets.constEnd(); ++it) {
        QAction *_act = new QAction(presetIcon(it.key()), it.key(), dispMenu);
        dispMenu->addAction(_act);
    }

    dispMenu->exec(m_types->mapToGlobal(point));
}

void DialogDbCreation::handlePresetClicked(const QAction *act)
{
    if (!isFilterCreating()) {
        setFilterCreation(FC_Enabled);
    }

    // act->text() may contain an ampersand in some Qt versions (6.7.2 for example)
    const QString __s = act->toolTip();
    m_types->setChecked(s_presets.value(__s));

    QRadioButton *p_rb = __s.startsWith('!') ? ui->rb_ignore
                                             : ui->rb_include;
    p_rb->setChecked(true);
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
    return (m_mode == FC_Enabled);
}

void DialogDbCreation::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        && m_types->hasFocus())
    {
        activateItem(m_types->currentItem());
        return;
    }

    if (event->key() == Qt::Key_Escape
        && isFilterCreating()
        && ui->tabWidget->currentIndex() == 0)
    {
        if (m_types->hasChecked())
            clearChecked();
        else
            setFilterCreation(FC_Disabled);
        return;
    }

    QDialog::keyPressEvent(event);
}
