/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
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
    setWindowIcon(QIcon(":/veretino.png"));

    icons_.setTheme(palette());
    setFileName(filePath_);
    showLineSizeAlgo();

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
    setWindowTitle("Checksums Match");
    setIcon(icons_.icon(FileStatus::Matched));

    ui->textChecksum->setTextColor(QColor("green"));
    ui->textChecksum->setText(values_.checksum);
}

void DialogFileProcResult::setModeMismatched()
{
    setWindowTitle("Checksums do not match");
    setIcon(icons_.icon(FileStatus::Mismatched));

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
    setWindowTitle("Checksum calculated");
    setIcon(icons_.icon(Icons::HashFile));

    ui->textChecksum->setText(values_.checksum);

    ui->buttonBox->clear();
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

    addButtonCopy();
    addButtonSave();
}

void DialogFileProcResult::setModeCopied()
{
    setWindowTitle("Copied to clipboard");
    setIcon(icons_.icon(Icons::Paste));

    ui->textChecksum->setText(values_.checksum);

    QGuiApplication::clipboard()->setText(values_.checksum);
}

void DialogFileProcResult::setModeStored()
{
    setWindowTitle("The checksum is saved");
    setIcon(icons_.icon(Icons::Save));
    hideLineSizeAlgo();

    ui->textChecksum->setText(values_.checksum);

    ui->buttonBox->clear();
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);
}

void DialogFileProcResult::setModeUnstored()
{
    setWindowTitle("Unable to create a summary file");
    setIcon(icons_.icon(Icons::DocClose));
    hideLineSizeAlgo();

    ui->textChecksum->setText(values_.checksum);

    ui->buttonBox->clear();
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
    addButtonCopy();
}

void DialogFileProcResult::setIcon(const QIcon &icon)
{
    ui->labelIcon->setPixmap(icon.pixmap(64, 64));
}

void DialogFileProcResult::setFileName(const QString &filePath)
{
    ui->labelFileName->setText("File: " + paths::basicName(filePath));
}

void DialogFileProcResult::showLineSizeAlgo()
{
    ui->labelFileSize->setVisible(true);
    ui->labelAlgo->setVisible(true);

    ui->labelFileSize->setText("Size: " + format::dataSizeReadable(values_.size));
    ui->labelAlgo->setText("Algorithm: " + format::algoToStr(values_.checksum.length()));
}

void DialogFileProcResult::hideLineSizeAlgo()
{
    ui->labelFileSize->setVisible(false);
    ui->labelAlgo->setVisible(false);
}

void DialogFileProcResult::addButtonCopy()
{
    QPushButton *buttonCopy = ui->buttonBox->addButton("Copy", QDialogButtonBox::AcceptRole);
    buttonCopy->setIcon(icons_.icon(Icons::Copy));

    connect(buttonCopy, &QPushButton::clicked, this, [=]{QGuiApplication::clipboard()->setText(values_.checksum);});
}

void DialogFileProcResult::addButtonSave()
{
    QPushButton *buttonSave = ui->buttonBox->addButton("Save", QDialogButtonBox::ActionRole);
    buttonSave->setIcon(icons_.icon(Icons::Save));

    connect(buttonSave, &QPushButton::clicked, this, &DialogFileProcResult::makeSumFile);
}

void DialogFileProcResult::makeSumFile()
{
    if (filePath_.isEmpty() || !tools::canBeChecksum(values_.checksum))
        return;

    QCryptographicHash::Algorithm algo = tools::algorithmByStrLen(values_.checksum.length());
    QString ext = format::algoToStr(algo, false);
    QString sumFile = QString("%1.%2").arg(filePath_, ext);

    setFileName(sumFile);

    QFile file(sumFile);

    if (file.open(QFile::WriteOnly)
        && (file.write(QString("%1 *%2").arg(values_.checksum, paths::basicName(filePath_)).toUtf8()) > 0)) {

        setModeStored();
    }
    else
        setModeUnstored();
}
