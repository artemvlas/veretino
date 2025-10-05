/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogfileprocresult.h"
#include "ui_dialogfileprocresult.h"
#include "tools.h"
#include "pathstr.h"
#include <QPushButton>
#include <QFile>
#include <QClipboard>

DialogFileProcResult::DialogFileProcResult(const QString &filePath, const FileValues &values, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogFileProcResult)
    , filePath_(filePath)
    , values_(values)
{
    ui->setupUi(this);
    setWindowIcon(IconProvider::appIcon());

    icons_.setTheme(palette());
    setFileName(filePath_);
    setExtLineVisible(values_.hash_purpose != FileValues::HashingPurpose::SaveToDigestFile);

    switch (values_.hash_purpose) {
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
        makeSumFile();
        break;
    default:
        break;
    }
}

DialogFileProcResult::~DialogFileProcResult()
{
    delete ui;
}

void DialogFileProcResult::setModeMatched()
{
    setWindowTitle(QStringLiteral(u"Checksums Match"));
    setIcon(FileStatus::Matched);

    ui->textChecksum->setTextColor(QColor("green"));
    ui->textChecksum->setText(values_.checksum);
}

void DialogFileProcResult::setModeMismatched()
{
    setWindowTitle(QStringLiteral(u"Checksums do not match"));
    setIcon(FileStatus::Mismatched);

    ui->textChecksum->setTextColor(QColor("red"));
    ui->textChecksum->setText(values_.checksum);

    QTextEdit *reChecksum = new QTextEdit(this);
    reChecksum->setReadOnly(true);
    reChecksum->setMinimumHeight(45);
    reChecksum->setTextColor(QColor("green"));
    reChecksum->setText(values_.reChecksum);
    ui->verticalLayout->addWidget(reChecksum);
}

void DialogFileProcResult::setModeComputed()
{
    setWindowTitle(QStringLiteral(u"Checksum calculated"));
    setIcon(Icons::HashFile);

    ui->textChecksum->setText(values_.checksum);

    ui->buttonBox->clear();
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

    addButtonCopy();
    addButtonSave();
}

void DialogFileProcResult::setModeCopied()
{
    setWindowTitle(QStringLiteral(u"Copied to clipboard"));
    setIcon(Icons::Paste);

    ui->textChecksum->setText(values_.checksum);

    QGuiApplication::clipboard()->setText(values_.checksum);
}

void DialogFileProcResult::setModeStored()
{
    setWindowTitle(QStringLiteral(u"The checksum is saved"));
    setIcon(Icons::Save);
    setExtLineVisible(false);

    ui->textChecksum->setText(values_.checksum);

    ui->buttonBox->clear();
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);
}

void DialogFileProcResult::setModeUnstored()
{
    setWindowTitle(QStringLiteral(u"Unable to create a summary file"));
    setIcon(Icons::DocClose);
    setExtLineVisible(false);

    ui->textChecksum->setText(values_.checksum);

    ui->buttonBox->clear();
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
    addButtonCopy();
}

void DialogFileProcResult::setIcon(Icons icon)
{
    ui->labelIcon->setPixmap(icons_.pixmap(icon));
}

void DialogFileProcResult::setIcon(FileStatus status)
{
    ui->labelIcon->setPixmap(icons_.pixmap(status));
}

void DialogFileProcResult::setFileName(const QString &filePath)
{
    ui->labelFileName->setText(QStringLiteral(u"File: ") + pathstr::basicName(filePath));
}

void DialogFileProcResult::setExtLineVisible(bool visible)
{
    ui->labelFileSize->setVisible(visible);
    ui->labelAlgo->setVisible(visible);

    // Hashing Speed
    const bool isHashTimeRecorded = (values_.hash_time >= 0);
    ui->labelSpeed->setVisible(visible && isHashTimeRecorded);

    if (!visible)
        return;

    ui->labelFileSize->setText(QStringLiteral(u"Size: ") + format::dataSizeReadable(values_.size));
    ui->labelAlgo->setText(QStringLiteral(u"Algorithm: ") + format::algoToStr(values_.checksum.length()));

    if (isHashTimeRecorded) {
        ui->labelSpeed->setText(QStringLiteral(u"Speed: ")
                                + format::processSpeed(values_.hash_time, values_.size));
    }
}

void DialogFileProcResult::addButtonCopy()
{
    QPushButton *buttonCopy = ui->buttonBox->addButton(QStringLiteral(u"Copy"), QDialogButtonBox::AcceptRole);
    buttonCopy->setIcon(icons_.icon(Icons::Copy));

    connect(buttonCopy, &QPushButton::clicked, this, [=]{ QGuiApplication::clipboard()->setText(values_.checksum); });
}

void DialogFileProcResult::addButtonSave()
{
    QPushButton *buttonSave = ui->buttonBox->addButton(QStringLiteral(u"Save"), QDialogButtonBox::ActionRole);
    buttonSave->setIcon(icons_.icon(Icons::Save));

    connect(buttonSave, &QPushButton::clicked, this, &DialogFileProcResult::makeSumFile);
}

void DialogFileProcResult::makeSumFile()
{
    const QString &chsum = values_.checksum;

    if (filePath_.isEmpty() || !tools::canBeChecksum(chsum))
        return;

    const QString sumFile = paths::digestFilePath(filePath_, chsum.size());
    const QString strToWrite = tools::joinStrings(chsum, pathstr::basicName(filePath_), QStringLiteral(u" *"));

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

void DialogFileProcResult::verify()
{
    if (values_.checksum.isEmpty() || values_.reChecksum.isEmpty()) {
        setWindowTitle("not enough data!");
        return;
    }

    const int compare = values_.checksum.compare(values_.reChecksum, Qt::CaseInsensitive);

    if (compare == 0) { // Matched
        setModeMatched();
    } else {
        setModeMismatched();
    }
}
