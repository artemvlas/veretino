#include "lineedit.h"
#include <QTimer>

LineEdit::LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    connect(this, &LineEdit::textEdited, this, &LineEdit::delayed_process_s_edited);
}

void LineEdit::setEditedDelay(int delay)
{
    _delay_s_edited = delay;
}

void LineEdit::delayed_process_s_edited()
{
    if (_queued_s_edited)
        return;

    _queued_s_edited = true;
    QTimer::singleShot(_delay_s_edited, this, &LineEdit::send_edited);
}

void LineEdit::send_edited()
{
    emit edited(text());
    _queued_s_edited = false;
}
