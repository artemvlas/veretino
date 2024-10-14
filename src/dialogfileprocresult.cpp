/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogfileprocresult.h"
#include "ui_dialogfileprocresult.h"
#include "tools.h"
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
    setExtLineVisible(values_.status != FileStatus::ToSumFile);

    switch (values_.status) {
    case FileStatus::Matched:
        setModeMatched();
        break;
    case FileStatus::Mismatched:
        setModeMismatched();
        break;
    case FileStatus::Computed:
        setModeComputed();
        break;
    case FileStatus::ToClipboard:
        setModeCopied();
        break;
    case FileStatus::ToSumFile:
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

void DialogFileProcResult::setIcon(Icons _icon)
{
    ui->labelIcon->setPixmap(icons_.pixmap(_icon));
}

void DialogFileProcResult::setIcon(FileStatus _status)
{
    ui->labelIcon->setPixmap(icons_.pixmap(_status));
}

void DialogFileProcResult::setFileName(const QString &filePath)
{
    ui->labelFileName->setText(QStringLiteral(u"File: ") + paths::basicName(filePath));
}

void DialogFileProcResult::setExtLineVisible(bool visible)
{
    ui->labelFileSize->setVisible(visible);
    ui->labelAlgo->setVisible(visible);

    if (visible) {
        ui->labelFileSize->setText(QStringLiteral(u"Size: ") + format::dataSizeReadable(values_.size));
        ui->labelAlgo->setText(QStringLiteral(u"Algorithm: ") + format::algoToStr(values_.checksum.length()));
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
    using namespace tools;
    const QString &_chsum = values_.checksum;

    if (filePath_.isEmpty() || !canBeChecksum(_chsum))
        return;

    const QString ext = format::algoToStr(_chsum.size(), false);
    const QString sumFile = joinStrings(filePath_, ext, u'.');
    const QString strToWrite = joinStrings(_chsum, paths::basicName(filePath_), QStringLiteral(u" *"));

    setFileName(sumFile);

    QFile file(sumFile);

    if (file.open(QFile::WriteOnly)
        && file.write(strToWrite.toUtf8()))
    {
        setModeStored();
    }
    else {
        setModeUnstored();
    }
}
