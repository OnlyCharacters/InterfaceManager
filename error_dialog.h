#ifndef ERROR_DIALOG_H
#define ERROR_DIALOG_H

#include <QDialog>

namespace Ui {
class error_dialog;
}

class error_dialog : public QDialog
{
    Q_OBJECT

public:
    explicit error_dialog(QWidget *parent = nullptr);
    ~error_dialog();
    void setContent(const QString content);

private slots:
    void on_btn_accept_clicked();

private:
    Ui::error_dialog *ui;
};

#endif // ERROR_DIALOG_H
