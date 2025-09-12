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
    , m_ui(new Ui::DialogDbStatus)
    , m_data(data)
{
    m_ui->setupUi(this);
    setWindowIcon(IconProvider::appIcon());

    if (m_data->isDbFileState(DbFileState::NotSaved))
        setWindowTitle(windowTitle() + QStringLiteral(u" [unsaved]"));

    if (m_data->isImmutable())
        setWindowTitle(windowTitle() + QStringLiteral(u" [const]"));

    m_ui->inpComment->setMaxLength(MAX_LENGTH_COMMENT);

    setLabelsInfo();
    setTabsInfo();
    setVisibleTabs();
    connections();
}

DialogDbStatus::~DialogDbStatus()
{
    delete m_ui;
}

void DialogDbStatus::connections()
{
    connect(m_ui->labelDbFileName, &ClickableLabel::doubleClicked, this, [=]{ paths::browsePath(pathstr::parentFolder(m_data->m_metadata.dbFilePath)); });
    connect(m_ui->labelWorkDir, &ClickableLabel::doubleClicked, this, [=]{ paths::browsePath(m_data->m_metadata.workDir); });
}

void DialogDbStatus::setLabelsInfo()
{
    const MetaData &meta = m_data->m_metadata;

    m_ui->labelDbFileName->setText(m_data->databaseFileName());
    m_ui->labelDbFileName->setToolTip(meta.dbFilePath);
    m_ui->labelAlgo->setText(QStringLiteral(u"Algorithm: ") + format::algoToStr(meta.algorithm));
    m_ui->labelWorkDir->setToolTip(meta.workDir);

    if (!m_data->isWorkDirRelative())
        m_ui->labelWorkDir->setText(QStringLiteral(u"WorkDir: Specified"));

    // datetime
    const VerDateTime &dt = meta.datetime;

    if (dt.hasValue(VerDateTime::Updated)) {
        m_ui->labelDateTime_Update->setText(dt.valueWithHint(VerDateTime::Updated));
        m_ui->labelDateTime_Update->setToolTip(dt.valueWithHint(VerDateTime::Created));
    } else {
        m_ui->labelDateTime_Update->setText(dt.valueWithHint(VerDateTime::Created));
    }

    m_ui->labelDateTime_Check->setText(dt.valueWithHint(VerDateTime::Verified));
}

void DialogDbStatus::setTabsInfo()
{
    IconProvider icons(palette()); // to set tabs icons

    // tab Content
    m_ui->labelContentNumbers->setStyleSheet(QStringLiteral(u"QLabel { font-family: monospace; }"));
    m_ui->labelContentNumbers->setText(infoContent().join('\n'));
    m_ui->tabWidget->setTabIcon(TabListed, icons.icon(Icons::Database));

    // tab Filter
    m_ui->tabWidget->setTabEnabled(TabFilter, m_data->isFilterApplied());
    if (m_data->isFilterApplied()) {
        m_ui->tabWidget->setTabIcon(TabFilter, icons.icon(Icons::Filter));

        const FilterRule &filter = m_data->m_metadata.filter;
        m_ui->labelFiltersInfo->setStyleSheet(format::coloredText(filter.isFilter(FilterRule::Ignore)));

        const QString exts = filter.extensionString();
        const QString str_mode = filter.isFilter(FilterRule::Include) ? QStringLiteral(u"Included:\n")
                                                                      : QStringLiteral(u"Ignored:\n");
        m_ui->labelFiltersInfo->setText(str_mode + exts);
    }

    // tab Verification
    m_ui->tabWidget->setTabEnabled(TabVerification, m_data->contains(FileStatus::CombChecked));
    if (m_data->contains(FileStatus::CombChecked)) {
        m_ui->tabWidget->setTabIcon(TabVerification, icons.icon(Icons::DoubleGear));
        m_ui->labelVerification->setText(infoVerification().join('\n'));
    }

    // tab Result
    m_ui->tabWidget->setTabEnabled(TabChanges, !isJustCreated() && m_data->contains(FileStatus::CombDbChanged));
    if (m_ui->tabWidget->isTabEnabled(TabChanges)) {
        m_ui->tabWidget->setTabIcon(TabChanges, icons.icon(Icons::Update));
        m_ui->labelResult->setText(infoChanges().join('\n'));
    }

    // tab Comment
    m_ui->inpComment->setText(m_data->m_metadata.comment);
}

QStringList DialogDbStatus::infoContent()
{
    if (isCreating())
        return { QStringLiteral(u"The checksum list is being calculated...") };

    const Numbers &num = m_data->m_numbers;
    QStringList contentNumbers;
    const NumSize nAvail = num.values(FileStatus::CombAvailable);
    const int numChecksums = num.numberOf(FileStatus::CombHasChecksum);

    if (isJustCreated() || (numChecksums != nAvail.number)) {
        QString _storedChecksums = tools::joinStrings(QStringLiteral(u"Stored checksums:"), numChecksums);

        if (isJustCreated())
            contentNumbers << format::addStrInParentheses(_storedChecksums, format::dataSizeReadable(nAvail.total_size));
        else
            contentNumbers << _storedChecksums;
    }

    const NumSize nUnr = num.values(FileStatus::CombUnreadable);
    if (nUnr)
        contentNumbers.append(tools::joinStrings(QStringLiteral(u"Unreadable files:"), nUnr.number));

    if (isJustCreated())
        return contentNumbers;

    if (nAvail)
        contentNumbers.append(QStringLiteral(u"Available: ") + format::filesNumSize(nAvail));
    else
        contentNumbers.append("NO FILES available to check");

    // [experimental]
    const NumSize nMod = num.values(FileStatus::NotCheckedMod);
    if (nMod)
        contentNumbers << tools::joinStrings(QStringLiteral(u"Modified: "), nMod.number);
        // addit. space is needed for align. with "Available: " (monospace fonts)
    // [exp.]

    contentNumbers.append(QString());
    contentNumbers.append(QStringLiteral(u"***"));

    contentNumbers.append(QStringLiteral(u"New:     ") + format::filesNumSize(num, FileStatus::New));
    contentNumbers.append(QStringLiteral(u"Missing: ") + format::filesNumber(num.numberOf(FileStatus::Missing)));

    if (m_data->isImmutable()) {
        contentNumbers.append(QString());
        contentNumbers.append(QStringLiteral(u"No changes are allowed"));
    }

    return contentNumbers;
}

QStringList DialogDbStatus::infoVerification()
{
    QStringList result;
    const Numbers &num = m_data->m_numbers;
    const int n_available = num.numberOf(FileStatus::CombAvailable);
    const int n_mismatch = num.numberOf(FileStatus::Mismatched);

    if (m_data->isAllChecked()) {
        const int n_checksums = num.numberOf(FileStatus::CombHasChecksum);

        if (n_mismatch) {
            result.append(QString("☒ %1 mismatches out of %2 available files")
                              .arg(n_mismatch)
                              .arg(n_available));
        }
        else if (n_checksums == n_available)
            result.append(QString("✓ ALL %1 stored checksums matched").arg(n_checksums));
        else
            result.append(QString("✓ All %1 available files matched the stored checksums").arg(n_available));
    }
    else if (m_data->contains(FileStatus::CombChecked)) {
        // to account for added and updated files, the total number in parentheses is used
        const int n_added_updated = num.numberOf(FileStatus::Added | FileStatus::Updated);

        // info str
        const int n_checked = num.numberOf(FileStatus::CombChecked);
        result.append(QString("%1%2 out of %3 files were checked")
                          .arg(n_checked)
                          .arg(n_added_updated > 0 ? format::inParentheses(n_checked + n_added_updated) : QString())
                          .arg(n_available));

        result.append(QString());
        if (n_mismatch)
            result.append(format::filesNumber(n_mismatch) + QStringLiteral(u" MISMATCHED"));
        else
            result.append(QStringLiteral(u"No Mismatches found"));

        const int n_matched = num.numberOf(FileStatus::Matched);
        if (n_matched) {
            result.append(QString("%1%2 files matched")
                              .arg(n_matched)
                              .arg(n_added_updated > 0 ? format::inParentheses(n_matched + n_added_updated) : QString()));
        }
    }

    return result;
}

QStringList DialogDbStatus::infoChanges()
{
    const Numbers &numb = m_data->m_numbers;
    QStringList result;

    // This list is used instead of Numbers::statuses() to order the strings
    static const QList<FileStatus> statuses { FileStatus::Moved, FileStatus::Added, FileStatus::Removed,
                                              FileStatus::Updated, FileStatus::Imported };

    static const FileStatuses flag_numsize = (FileStatus::Added | FileStatus::Moved); // for these the size will be shown

    for (const FileStatus st : statuses) {
        const NumSize ns = numb.values(st);
        if (ns) {
            const QString str = (st & flag_numsize) ? format::filesNumSize(ns)
                                                    : QString::number(ns.number);

            result << tools::joinStrings(tools::enumToString(st), str, Lit::s_sepColonSpace);
        }
    }

    return result;
}

void DialogDbStatus::setVisibleTabs()
{
    // hide disabled tabs
    for (int var = 0; var < m_ui->tabWidget->count(); ++var) {
        m_ui->tabWidget->setTabVisible(var, m_ui->tabWidget->isTabEnabled(var));
    }
}
void DialogDbStatus::selectCurTab()
{
    // selecting the tab to open
    Tabs curTab = TabListed;
    if (m_ui->tabWidget->isTabEnabled(TabChanges))
        curTab = TabChanges;
    else if (m_ui->tabWidget->isTabEnabled(TabVerification))
        curTab = TabVerification;

    m_ui->tabWidget->setCurrentIndex(curTab);
}

void DialogDbStatus::setCurrentTab(Tabs tab)
{
    if (tab != TabAutoSelect && m_ui->tabWidget->isTabEnabled(tab)) {
        m_ui->tabWidget->setCurrentIndex(tab);
        autoTabSelection = false;
    }
}

bool DialogDbStatus::isCreating() const
{
    return m_data->isDbFileState(MetaData::NoFile);
}

bool DialogDbStatus::isJustCreated() const
{
    return m_data->isDbFileState(MetaData::Created);
}

QString DialogDbStatus::getComment() const
{
    return m_ui->inpComment->text();
}

void DialogDbStatus::showEvent(QShowEvent *event)
{
    if (autoTabSelection)
        selectCurTab();

    QDialog::showEvent(event);
}
