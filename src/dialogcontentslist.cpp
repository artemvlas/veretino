/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogcontentslist.h"
#include "ui_dialogcontentslist.h"
#include "tools.h"
#include <QPushButton>
#include <QDebug>

DialogContentsList::DialogContentsList(const QString &folderPath, const FileTypeList &extList, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogContentsList)
{
    ui->setupUi(this);
    ui->f_selected_info->setVisible(false);
    ui->types_->setColumnWidth(ItemFileType::ColumnType, 130);
    ui->types_->setColumnWidth(ItemFileType::ColumnFilesNumber, 130);
    ui->types_->sortByColumn(ItemFileType::ColumnTotalSize, Qt::DescendingOrder);

    ui->labelFolderName->setText(format::shortenPath(folderPath));
    ui->labelFolderName->setToolTip(folderPath);
    ui->chbTop10->setVisible(extList.count() > 15);

    setTotalInfo(extList);
    ui->types_->setItems(extList);
    ui->types_->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ui->l_selected->setStyleSheet(format::coloredText(false));
    ui->l_unselected->setStyleSheet(format::coloredText(true));

    connections();
}

DialogContentsList::~DialogContentsList()
{
    delete ui;
}

void DialogContentsList::connections()
{
    connect(ui->types_, &WidgetFileTypes::itemSelectionChanged, this, &DialogContentsList::updateSelectInfo);
    connect(ui->chbTop10, &QCheckBox::toggled, this, &DialogContentsList::setItemsVisibility);
    connect(ui->labelFolderName, &ClickableLabel::doubleClicked, this,
            [=]{ paths::browsePath(ui->labelFolderName->toolTip()); });
}

void DialogContentsList::setTotalInfo(const FileTypeList &extList)
{
    _n_total = Files::totalListed(extList);
    ui->labelTotal->setText(QString("Total: %1 types, %2 ")
                                .arg(extList.count())
                                .arg(format::filesNumSize(_n_total)));
}

void DialogContentsList::setItemsVisibility(bool isTop10Checked)
{
    QString lbl;

    if (isTop10Checked) {
        ui->types_->clearSelection();
        ui->types_->hideExtra();
        lbl = QStringLiteral(u"Top10: ") + format::filesNumSize(ui->types_->numSizeVisible());
    } else {
        ui->types_->showAllItems();
        lbl = QStringLiteral(u"Top10");
    }

    ui->chbTop10->setText(lbl);
}

void DialogContentsList::updateSelectInfo()
{
    const QList<QTreeWidgetItem *> selected = ui->types_->selectedItems();
    ui->f_selected_info->setVisible(selected.size() > 1);

    if (!ui->f_selected_info->isVisible())
        return;

    NumSize nSel;

    for (QTreeWidgetItem *itm : selected) {
        const ItemFileType *item = static_cast<ItemFileType *>(itm);
        nSel += item->numSize();
    }

    ui->l_selected->setText(format::filesNumSize(nSel));

    const NumSize nUnsel = _n_total - nSel;
    const bool hasUnsel = (bool)nUnsel;

    // hide 'unselected' label if no items
    ui->l_unselected->setVisible(hasUnsel);
    ui->l_sep->setVisible(hasUnsel);

    if (hasUnsel) {
        ui->l_unselected->setText(format::filesNumSize(nUnsel));
    }
}
