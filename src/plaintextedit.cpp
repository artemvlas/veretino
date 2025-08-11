/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "plaintextedit.h"

PlainTextEdit::PlainTextEdit(QWidget *parent)
    : QPlainTextEdit(parent)
{
    connections();
}

PlainTextEdit::PlainTextEdit(const QString &text, QWidget *parent)
    : QPlainTextEdit(parent)
{
    connections();
    setPlainText(text);
}

PlainTextEdit::PlainTextEdit(const int maxTextLength, QWidget *parent)
    : QPlainTextEdit(parent)
{
    connections();
    setMaxLength(maxTextLength);
}

void PlainTextEdit::connections()
{
    connect(this, &QPlainTextEdit::textChanged, this, &PlainTextEdit::cutExcess);
}

void PlainTextEdit::setMaxLength(int length)
{
    if (length > 0)
        mMaxLength = length;
}

void PlainTextEdit::setText(const QString &text)
{
    setPlainText(text);
}

int PlainTextEdit::maxLength() const
{
    return mMaxLength;
}

QString PlainTextEdit::text() const
{
    return toPlainText();
}

void PlainTextEdit::cutExcess()
{
    if (mMaxLength <= 0)
        return;

    const QString curText = toPlainText();

    if (curText.length() > mMaxLength) {
        setPlainText(curText.left(curText.length() - 1));
        moveCursor(QTextCursor::End);
    }
}
