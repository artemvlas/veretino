/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QPlainTextEdit>

class PlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit PlainTextEdit(QWidget *parent = nullptr);
    explicit PlainTextEdit(const QString &text, QWidget *parent = nullptr);
    explicit PlainTextEdit(const int maxTextLength, QWidget *parent = nullptr);

    void connections();
    void cutExcess();
    void setMaxLength(int length);
    void setText(const QString &text);
    int maxLength() const;
    QString text() const;

private:
    int mMaxLength = 0;
};

#endif // PLAINTEXTEDIT_H
