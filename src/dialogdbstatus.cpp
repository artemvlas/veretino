/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogdbstatus.h"
#include "ui_dialogdbstatus.h"
#include "clickablelabel.h"
#include <QFile>
#include <QDebug>
#include "iconprovider.h"
#include "tools.h"
#include "pathstr.h"

DialogDbStatus::DialogDbStatus(const DataContainer *data, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogDbStatus)
    , data_(data)
{
    ui->setupUi(this);
    setWindowIcon(IconProvider::appIcon());

    if (data_->isDbFileState(DbFileState::NotSaved))
        setWindowTitle(windowTitle() + QStringLiteral(u" [unsaved]"));

    if (data_->isImmutable())
        setWindowTitle(windowTitle() + QStringLiteral(u" [const]"));

    setLabelsInfo();
    setTabsInfo();
    setVisibleTabs();
    connections();
}

DialogDbStatus::~DialogDbStatus()
{
    delete ui;
}

void DialogDbStatus::connections()
{
    connect(ui->labelDbFileName, &ClickableLabel::doubleClicked, this, [=]{ paths::browsePath(pathstr::parentFolder(data_->metaData_.dbFilePath)); });
    connect(ui->labelWorkDir, &ClickableLabel::doubleClicked, this, [=]{ paths::browsePath(data_->metaData_.workDir); });
}

void DialogDbStatus::setLabelsInfo()
{
    const MetaData &_meta = data_->metaData_;

    ui->labelDbFileName->setText(QFileInfo::exists(_meta.dbFilePath) ? data_->databaseFileName() : "no file yet");
    ui->labelDbFileName->setToolTip(_meta.dbFilePath);
    ui->labelAlgo->setText(QStringLiteral(u"Algorithm: ") + format::algoToStr(_meta.algorithm));
    ui->labelWorkDir->setToolTip(_meta.workDir);

    if (!data_->isWorkDirRelative())
        ui->labelWorkDir->setText(QStringLiteral(u"WorkDir: Specified"));

    // datetime
    const VerDateTime &_dt = _meta.datetime;

    if (_dt.m_updated.isEmpty())
        ui->labelDateTime_Update->setText(_dt.m_created);
    else {
        ui->labelDateTime_Update->setText(_dt.m_updated);
        ui->labelDateTime_Update->setToolTip(_dt.m_created);
    }

    ui->labelDateTime_Check->setText(_dt.m_verified);
}

void DialogDbStatus::setTabsInfo()
{
    IconProvider icons(palette()); // to set tabs icons

    // tab Content
    ui->labelContentNumbers->setStyleSheet(QStringLiteral(u"QLabel { font-family: monospace; }"));
    ui->labelContentNumbers->setText(infoContent().join('\n'));
    ui->tabWidget->setTabIcon(TabListed, icons.icon(Icons::Database));

    // tab Filter
    ui->tabWidget->setTabEnabled(TabFilter, data_->isFilterApplied());
    if (data_->isFilterApplied()) {
        ui->tabWidget->setTabIcon(TabFilter, icons.icon(Icons::Filter));

        const FilterRule &_filter = data_->metaData_.filter;
        ui->labelFiltersInfo->setStyleSheet(format::coloredText(_filter.isFilter(FilterRule::Ignore)));

        const QString _exts = _filter.extensionString();
        const QString _str_mode = _filter.isFilter(FilterRule::Include) ? QStringLiteral(u"Included:\n")
                                                                        : QStringLiteral(u"Ignored:\n");
        ui->labelFiltersInfo->setText(_str_mode + _exts);
    }

    // tab Verification
    ui->tabWidget->setTabEnabled(TabVerification, data_->contains(FileStatus::CombChecked));
    if (data_->contains(FileStatus::CombChecked)) {
        ui->tabWidget->setTabIcon(TabVerification, icons.icon(Icons::DoubleGear));
        ui->labelVerification->setText(infoVerification().join('\n'));
    }

    // tab Result
    ui->tabWidget->setTabEnabled(TabChanges, !isJustCreated() && data_->contains(FileStatus::CombDbChanged));
    if (ui->tabWidget->isTabEnabled(TabChanges)) {
        ui->tabWidget->setTabIcon(TabChanges, icons.icon(Icons::Update));
        ui->labelResult->setText(infoChanges().join('\n'));
    }
}

QStringList DialogDbStatus::infoContent()
{
    if (isCreating())
        return { QStringLiteral(u"The checksum list is being calculated...") };

    const Numbers &_num = data_->numbers_;
    QStringList contentNumbers;
    const NumSize _n_avail = _num.values(FileStatus::CombAvailable);
    const int numChecksums = _num.numberOf(FileStatus::CombHasChecksum);

    if (isJustCreated() || (numChecksums != _n_avail._num)) {
        QString _storedChecksums = tools::joinStrings(QStringLiteral(u"Stored checksums:"), numChecksums);

        if (isJustCreated())
            contentNumbers << format::addStrInParentheses(_storedChecksums, format::dataSizeReadable(_n_avail._size));
        else
            contentNumbers << _storedChecksums;
    }

    const NumSize _n_unr = _num.values(FileStatus::CombUnreadable);
    if (_n_unr)
        contentNumbers.append(tools::joinStrings(QStringLiteral(u"Unreadable files:"), _n_unr._num));

    if (isJustCreated())
        return contentNumbers;

    if (_n_avail)
        contentNumbers.append(QStringLiteral(u"Available: ") + format::filesNumSize(_n_avail));
    else
        contentNumbers.append("NO FILES available to check");

    // [experimental]
    const NumSize _n_mod = _num.values(FileStatus::NotCheckedMod);
    if (_n_mod)
        contentNumbers << tools::joinStrings(QStringLiteral(u"Modified: "), _n_mod._num);
        // addit. space is needed for align. with "Available: " (monospace fonts)
    // [exp.]

    contentNumbers.append(QString());
    contentNumbers.append(QStringLiteral(u"***"));

    contentNumbers.append(QStringLiteral(u"New:     ") + format::filesNumSize(_num, FileStatus::New));
    contentNumbers.append(QStringLiteral(u"Missing: ") + format::filesNumber(_num.numberOf(FileStatus::Missing)));

    if (data_->isImmutable()) {
        contentNumbers.append(QString());
        contentNumbers.append(QStringLiteral(u"No changes are allowed"));
    }

    return contentNumbers;
}

QStringList DialogDbStatus::infoVerification()
{
    QStringList result;
    const Numbers &_num = data_->numbers_;
    const int _n_available = _num.numberOf(FileStatus::CombAvailable);
    const int _n_mismatch = _num.numberOf(FileStatus::Mismatched);

    if (data_->isAllChecked()) {
        const int _n_checksums = _num.numberOf(FileStatus::CombHasChecksum);

        if (_n_mismatch) {
            result.append(QString("☒ %1 mismatches out of %2 available files")
                              .arg(_n_mismatch)
                              .arg(_n_available));
        }
        else if (_n_checksums == _n_available)
            result.append(QString("✓ ALL %1 stored checksums matched").arg(_n_checksums));
        else
            result.append(QString("✓ All %1 available files matched the stored checksums").arg(_n_available));
    }
    else if (data_->contains(FileStatus::CombChecked)) {
        // to account for added and updated files, the total number in parentheses is used
        const int _n_added_updated = _num.numberOf(FileStatus::Added | FileStatus::Updated);

        // info str
        const int _n_checked = _num.numberOf(FileStatus::CombChecked);
        result.append(QString("%1%2 out of %3 files were checked")
                          .arg(_n_checked)
                          .arg(_n_added_updated > 0 ? format::inParentheses(_n_checked + _n_added_updated) : QString())
                          .arg(_n_available));

        result.append(QString());
        if (_n_mismatch)
            result.append(format::filesNumber(_n_mismatch) + QStringLiteral(u" MISMATCHED"));
        else
            result.append(QStringLiteral(u"No Mismatches found"));

        const int _n_matched = _num.numberOf(FileStatus::Matched);
        if (_n_matched) {
            result.append(QString("%1%2 files matched")
                              .arg(_n_matched)
                              .arg(_n_added_updated > 0 ? format::inParentheses(_n_matched + _n_added_updated) : QString()));
        }
    }

    return result;
}

QStringList DialogDbStatus::infoChanges()
{
    const Numbers &_numb = data_->numbers_;
    QStringList result;

    // This list is used instead of Numbers::statuses() to order the strings
    static const QList<FileStatus> _statuses { FileStatus::Moved, FileStatus::Added, FileStatus::Removed,
                                               FileStatus::Updated, FileStatus::Imported };

    static const FileStatuses _flag_numsize = (FileStatus::Added | FileStatus::Moved); // for these the size will be shown

    for (const FileStatus _st : _statuses) {
        const NumSize _ns = _numb.values(_st);
        if (_ns) {
            const QString _str = (_st & _flag_numsize) ? format::filesNumSize(_ns)
                                                       : QString::number(_ns._num);

            result << tools::joinStrings(tools::enumToString(_st), _str, Lit::s_sepColonSpace);
        }
    }

    return result;
}

void DialogDbStatus::setVisibleTabs()
{
    // hide disabled tabs
    for (int var = 0; var < ui->tabWidget->count(); ++var) {
        ui->tabWidget->setTabVisible(var, ui->tabWidget->isTabEnabled(var));
    }
}
void DialogDbStatus::selectCurTab()
{
    // selecting the tab to open
    Tabs curTab = TabListed;
    if (ui->tabWidget->isTabEnabled(TabChanges))
        curTab = TabChanges;
    else if (ui->tabWidget->isTabEnabled(TabVerification))
        curTab = TabVerification;

    ui->tabWidget->setCurrentIndex(curTab);
}

void DialogDbStatus::setCurrentTab(Tabs tab)
{
    if (tab != TabAutoSelect && ui->tabWidget->isTabEnabled(tab)) {
        ui->tabWidget->setCurrentIndex(tab);
        autoTabSelection = false;
    }
}

bool DialogDbStatus::isCreating() const
{
    return data_->isDbFileState(MetaData::NoFile);
}

bool DialogDbStatus::isJustCreated() const
{
    return data_->isDbFileState(MetaData::Created);
}

void DialogDbStatus::showEvent(QShowEvent *event)
{
    if (autoTabSelection)
        selectCurTab();

    QDialog::showEvent(event);
}
