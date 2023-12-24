#include "clickablelabel.h"
#include <QMouseEvent>

ClickableLabel::ClickableLabel(QWidget *parent)
    : QLabel(parent) {}

ClickableLabel::ClickableLabel(const QString &text, QWidget *parent)
    : QLabel(parent)
{
    setText(text);
}

void ClickableLabel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        emit clicked(this);
}

void ClickableLabel::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit doubleClicked(this);
}
