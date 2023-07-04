#include "error_dialog.h"
#include "ui_error_dialog.h"

error_dialog::error_dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::error_dialog)
{
    ui->setupUi(this);
    this->setWindowTitle("提示");
}

error_dialog::~error_dialog()
{
    delete ui;
}

void error_dialog::setContent(const QString content)
{
    this->ui->label->setText(content);
}

void error_dialog::on_btn_accept_clicked()
{
    this->accept();
}

