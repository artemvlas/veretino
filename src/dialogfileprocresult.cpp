/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "dialogfileprocresult.h"
#include "ui_dialogfileprocresult.h"
#include "tools.h"
#include <QPushButton>

DialogFileProcResult::DialogFileProcResult(const QString &filePath, const FileValues &values, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogFileProcResult)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/veretino.png"));
    icons_.setTheme(palette());
    setInfo(filePath, values);
}

DialogFileProcResult::~DialogFileProcResult()
{
    delete ui;
}

void DialogFileProcResult::setInfo(const QString &filePath, const FileValues &values)
{
    QIcon icon;
    QString titleText;

    switch (values.status) {
        case FileStatus::Matched:
        icon = icons_.icon(FileStatus::Matched);
            titleText = "Checksums Match";
            break;
        case FileStatus::Mismatched:
            icon = icons_.icon(FileStatus::Mismatched);
            titleText = "Checksums do not match";
            break;
        case FileStatus::Computed:
            icon = icons_.icon(Icons::HashFile);
            titleText = "Checksum calculated";
            setModeCalculated();
            break;
        case FileStatus::Copied:
            icon = icons_.icon(Icons::Paste);
            titleText = "Copied to clipboard";
            break;
        case FileStatus::Stored:
            icon = icons_.icon(Icons::Save);
            titleText = "The checksum is saved";
            ui->labelFileSize->setVisible(false);
            ui->labelAlgo->setVisible(false);
            break;
        case FileStatus::UnStored:
            icon = icons_.icon(Icons::DocClose);
            titleText = "Unable to create a summary file";
            setModeUnstored();
        default:
            break;
    }

    setWindowTitle(titleText);
    ui->labelIcon->setPixmap(icon.pixmap(64, 64));
    ui->labelFileName->setText("File: " + paths::basicName(filePath));
    ui->labelFileSize->setText("Size: " + format::dataSizeReadable(values.size));
    ui->labelAlgo->setText("Algorithm: " + format::algoToStr(values.checksum.length()));

    if (values.status == FileStatus::Matched) {
        ui->textChecksum->setTextColor(QColor("green"));
    }
    else if (values.status == FileStatus::Mismatched) {
        ui->textChecksum->setTextColor(QColor("red"));
        QTextEdit *reChecksum = new QTextEdit(this);
        reChecksum->setReadOnly(true);
        reChecksum->setMinimumHeight(40);
        reChecksum->setTextColor(QColor("green"));
        reChecksum->setText(values.reChecksum);
        ui->verticalLayout->addWidget(reChecksum);
    }

    ui->textChecksum->setText(values.checksum);
}

void DialogFileProcResult::setModeCalculated()
{
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
    QPushButton *buttonCopy = ui->buttonBox->addButton("Copy", QDialogButtonBox::AcceptRole);
    QPushButton *buttonSave = ui->buttonBox->addButton("Save", QDialogButtonBox::AcceptRole);

    buttonCopy->setIcon(icons_.icon(Icons::Copy));
    buttonSave->setIcon(icons_.icon(Icons::Save));

    connect(buttonCopy, &QPushButton::clicked, this, [=]{clickedButton = Copy;});
    connect(buttonSave, &QPushButton::clicked, this, [=]{clickedButton = Save;});
}

void DialogFileProcResult::setModeUnstored()
{
    ui->labelFileSize->setVisible(false);
    ui->labelAlgo->setVisible(false);

    ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
    QPushButton *buttonCopy = ui->buttonBox->addButton("Copy", QDialogButtonBox::AcceptRole);

    buttonCopy->setIcon(icons_.icon(Icons::Copy));

    connect(buttonCopy, &QPushButton::clicked, this, [=]{clickedButton = Copy;});
}
