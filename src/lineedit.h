#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QLineEdit>

class LineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit LineEdit(QWidget *parent = nullptr);

    // set the delay in milliseconds after which the <edited> signal will be sent
    void setEditedDelay(int delay);

    void delayed_process_s_edited();
    void send_edited();

private:
    bool _queued_s_edited = false;
    int _delay_s_edited = 500;

signals:
    void edited(const QString &text); // more gentle than QLineEdit::textEdited
}; // class LineEdit

#endif // LINEEDIT_H
