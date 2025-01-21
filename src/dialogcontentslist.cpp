/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogcontentslist.h"
#include "ui_dialogcontentslist.h"
#include "tools.h"
#include "pathstr.h"
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

    ui->labelFolderName->setText(pathstr::shortenPath(folderPath));
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
    QString __s;

    if (isTop10Checked) {
        ui->types_->clearSelection();
        ui->types_->hideExtra();
        __s = QStringLiteral(u"Top10: ") + format::filesNumSize(ui->types_->numSizeVisible());
    }
    else {
        ui->types_->showAllItems();
        __s = QStringLiteral(u"Top10");
    }

    ui->chbTop10->setText(__s);
}

void DialogContentsList::updateSelectInfo()
{
    const QList<QTreeWidgetItem *> _selected = ui->types_->selectedItems();
    ui->f_selected_info->setVisible(_selected.size() > 1);

    if (!ui->f_selected_info->isVisible())
        return;

    NumSize _n_sel;

    for (QTreeWidgetItem *_it : _selected) {
        const ItemFileType *_item = static_cast<ItemFileType *>(_it);
        _n_sel += _item->numSize();
    }

    ui->l_selected->setText(format::filesNumSize(_n_sel));
    ui->l_unselected->setText(format::filesNumSize(_n_total - _n_sel));
}
