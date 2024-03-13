#include "dialogfileprocresult.h"
#include "ui_dialogfileprocresult.h"
#include "tools.h"
#include <QPushButton>

DialogFileProcResult::DialogFileProcResult(const QString &fileName, const FileValues &values, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogFileProcResult)
{
    ui->setupUi(this);
    setInfo(fileName, values);
}

DialogFileProcResult::~DialogFileProcResult()
{
    delete ui;
}

void DialogFileProcResult::setInfo(const QString &fileName, const FileValues &values)
{
    QIcon icon;
    QString titleText;

    switch (values.status) {
        case FileStatus::Matched:
            icon = QIcon(":/icons/filestatus/matched.svg");
            titleText = "Checksums Match";
            break;
        case FileStatus::Mismatched:
            icon = QIcon(":/icons/filestatus/mismatched.svg");
            titleText = "Checksums do not match";
            break;
        case FileStatus::Added:
            icon = QIcon(":/icons/filestatus/processing.svg");
            titleText = "Checksum calculated";
            setModeCalculated();
            break;
        default:
            break;
    }

    setWindowTitle(titleText);
    ui->labelIcon->setPixmap(icon.pixmap(64, 64));
    ui->labelFileName->setText("File: " + fileName);
    ui->labelFileSize->setText("Size: " + format::dataSizeReadable(values.size));

    if (values.status == FileStatus::Matched) {
        ui->textChecksum->setTextColor(QColor("green"));
    }
    else if (values.status == FileStatus::Mismatched) {
        ui->textChecksum->setTextColor(QColor("red"));
        QTextEdit *reChecksum = new QTextEdit(this);
        reChecksum->setReadOnly(true);
        reChecksum->setMinimumHeight(50);
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

    QString themeFolder = tools::themeFolder(palette());
    buttonCopy->setIcon(QIcon(QString(":/icons/%1/copy.svg").arg(themeFolder)));
    buttonSave->setIcon(QIcon(QString(":/icons/%1/save.svg").arg(themeFolder)));

    connect(buttonCopy, &QPushButton::clicked, this, [=]{clickedButton = Copy;});
    connect(buttonSave, &QPushButton::clicked, this, [=]{clickedButton = Save;});
}
