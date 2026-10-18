/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogfileprocresult.h"
#include "ui_dialogfileprocresult.h"
#include "tools.h"
#include "pathstr.h"
#include "algostring.h"
#include "digeststring.h"
#include <QPushButton>
#include <QFile>
#include <QClipboard>

DialogFileProcResult::DialogFileProcResult(const QString &filePath, const FileValues &values, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::DialogFileProcResult)
    , m_filePath(filePath)
    , m_values(values)
{
    m_ui->setupUi(this);
    setWindowIcon(IconProvider::appIcon());
    m_icons.setTheme(palette());

    setFileName(m_filePath);
    setExtLineVisible(m_values.hash_purpose != FileValues::HashingPurpose::SaveToDigestFile);

    if (m_values.status & FileValues::CombCalcError) {
        setModeReadError();
        return;
    }

    switch (m_values.hash_purpose) {
    case FileValues::HashingPurpose::Generic:
        setModeComputed();
        break;
    case FileValues::HashingPurpose::Verify:
        verify();
        break;
    case FileValues::HashingPurpose::CopyToClipboard:
        setModeCopied();
        break;
    case FileValues::HashingPurpose::SaveToDigestFile:
        makeSumFileUnforce();
        break;
    default:
        break;
    }
}

DialogFileProcResult::~DialogFileProcResult()
{
    delete m_ui;
}

void DialogFileProcResult::setModeMatched()
{
    setWindowTitle(QStringLiteral(u"Checksums Match"));
    setIcon(FileStatus::Matched);

    m_ui->textChecksum->setTextColor(QColor("green"));
    m_ui->textChecksum->setText(m_values.checksum);
}

void DialogFileProcResult::setModeMismatched()
{
    setWindowTitle(QStringLiteral(u"Checksums do not match"));
    setIcon(FileStatus::Mismatched);

    m_ui->textChecksum->setTextColor(QColor("red"));
    m_ui->textChecksum->setText(m_values.checksum);

    QTextEdit *reChecksum = new QTextEdit(this);
    reChecksum->setReadOnly(true);
    reChecksum->setMinimumHeight(45);
    reChecksum->setTextColor(QColor("green"));
    reChecksum->setText(m_values.reChecksum);
    m_ui->verticalLayout->addWidget(reChecksum);
}

void DialogFileProcResult::setModeComputed()
{
    setWindowTitle(QStringLiteral(u"Checksum calculated"));
    setIcon(Icons::HashFile);

    m_ui->textChecksum->setText(m_values.checksum);

    m_ui->buttonBox->clear();
    m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

    addButtonCopy();
    addButtonSave();
}

void DialogFileProcResult::setModeCopied()
{
    setWindowTitle(QStringLiteral(u"Copied to clipboard"));
    setIcon(Icons::Paste);

    m_ui->textChecksum->setText(m_values.checksum);

    QGuiApplication::clipboard()->setText(m_values.checksum);
}

void DialogFileProcResult::setModeStored()
{
    setWindowTitle(QStringLiteral(u"The checksum is saved"));
    setIcon(Icons::Save);
    setExtLineVisible(false);

    m_ui->textChecksum->setText(m_values.checksum);

    m_ui->buttonBox->clear();
    m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);
}

void DialogFileProcResult::setModeUnstored()
{
    setWindowTitle(QStringLiteral(u"Unable to create a summary file"));
    setIcon(Icons::DocClose);
    setExtLineVisible(false);

    m_ui->textChecksum->setText(m_values.checksum);

    m_ui->buttonBox->clear();
    m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
    addButtonCopy();
}

void DialogFileProcResult::setModeDigestFileExists()
{
    setWindowTitle(QStringLiteral(u"Digest file exists"));
    setIcon(Icons::Update);
    setExtLineVisible(false);

    m_ui->textChecksum->setText(m_values.checksum);

    m_ui->buttonBox->clear();
    m_ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

    addButtonCopy();
    addButtonOverwrite();
}

void DialogFileProcResult::setModeReadError()
{
    resize(400, height());
    setIcon(m_values.status);
    m_ui->textChecksum->setVisible(false);

    switch (m_values.status) {
    case FileValues::Missing:
    case FileValues::Removed:
        setWindowTitle(QStringLiteral(u"File not found"));
        break;
    case FileValues::UnPermitted:
        setWindowTitle(QStringLiteral(u"No read permissions"));
        break;
    case FileValues::ReadError:
        setWindowTitle(QStringLiteral(u"Read Error"));
        break;
    default:
        break;
    }
}

void DialogFileProcResult::setIcon(Icons icon)
{
    m_ui->labelIcon->setPixmap(m_icons.pixmap(icon));
}

void DialogFileProcResult::setIcon(FileStatus status)
{
    m_ui->labelIcon->setPixmap(m_icons.pixmap(status));
}

void DialogFileProcResult::setFileName(const QString &filePath)
{
    m_ui->labelFileName->setText(QStringLiteral(u"File: ") + pathstr::entryName(filePath));
}

void DialogFileProcResult::setExtLineVisible(bool visible)
{
    const bool hasSize = (m_values.size >= 0);
    const bool hasHashTime = (m_values.hash_time >= 0);
    const bool hasDigest = !m_values.defaultChecksum().isEmpty();

    m_ui->labelFileSize->setVisible(visible && hasSize);
    m_ui->labelSpeed->setVisible(visible && hasHashTime);
    m_ui->labelAlgo->setVisible(visible && hasDigest);

    if (hasSize) {
        m_ui->labelFileSize->setText(QStringLiteral(u"Size: ")
                                     + format::dataSizeReadable(m_values.size));
    }

    if (hasHashTime) {
        m_ui->labelSpeed->setText(QStringLiteral(u"Speed: ")
                                  + format::processSpeed(m_values.size, m_values.hash_time));
    }

    if (hasDigest) {
        m_ui->labelAlgo->setText(QStringLiteral(u"Algorithm: ")
                                 + AlgoString::name(m_values.checksum.length()));
    }
}

void DialogFileProcResult::addButtonCopy()
{
    QPushButton *buttonCopy = m_ui->buttonBox->addButton(QStringLiteral(u"Copy"), QDialogButtonBox::AcceptRole);
    buttonCopy->setIcon(m_icons.icon(Icons::Copy));

    connect(buttonCopy, &QPushButton::clicked, this, [=]{ QGuiApplication::clipboard()->setText(m_values.checksum); });
}

void DialogFileProcResult::addButtonSave()
{
    QPushButton *buttonSave = m_ui->buttonBox->addButton(QStringLiteral(u"Save"), QDialogButtonBox::ActionRole);
    buttonSave->setIcon(m_icons.icon(Icons::Save));

    connect(buttonSave, &QPushButton::clicked, this, &DialogFileProcResult::makeSumFileUnforce);
}

void DialogFileProcResult::addButtonOverwrite()
{
    QPushButton *buttonOverwrite = m_ui->buttonBox->addButton(QStringLiteral(u"Overwrite"), QDialogButtonBox::ActionRole);
    buttonOverwrite->setStyleSheet(QStringLiteral(u"color: red"));
    buttonOverwrite->setIcon(m_icons.icon(Icons::Save));

    connect(buttonOverwrite, &QPushButton::clicked, this, &DialogFileProcResult::makeSumFile);
}

void DialogFileProcResult::makeSumFile()
{
    const QString &chsum = m_values.checksum;

    if (m_filePath.isEmpty() || !DigestStringView::isValid(chsum))
        return;

    const QString sumFile = digestFilePath();
    const QString strToWrite = tools::joinStrings(chsum, pathstr::entryName(m_filePath), QStringLiteral(u" *"));

    setFileName(sumFile);

    QFile file(sumFile);

    if (file.open(QFile::WriteOnly)
        && file.write(strToWrite.toUtf8()))
    {
        setModeStored();
    } else {
        setModeUnstored();
    }
}

void DialogFileProcResult::makeSumFileUnforce()
{
    const QString sumFile = digestFilePath();

    if (QFileInfo::exists(sumFile)) {
        setFileName(sumFile);
        setModeDigestFileExists();
    } else {
        makeSumFile();
    }
}

void DialogFileProcResult::verify()
{
    if (m_values.checksum.isEmpty() || m_values.reChecksum.isEmpty()) {
        setWindowTitle("not enough data!");
        return;
    }

    const int compare = m_values.checksum.compare(m_values.reChecksum, Qt::CaseInsensitive);

    if (compare == 0) { // Matched
        setModeMatched();
    } else {
        setModeMismatched();
    }
}

QString DialogFileProcResult::digestFilePath() const
{
    return paths::digestFilePath(m_filePath, m_values.checksum.size());
}