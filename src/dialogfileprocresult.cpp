#include "dialogfileprocresult.h"
#include "ui_dialogfileprocresult.h"
#include "tools.h"

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
    else if (values.status == FileStatus::Added) {

    }

    ui->textChecksum->setText(values.checksum);
}
