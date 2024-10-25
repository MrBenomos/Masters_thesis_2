#include "main_widget.h"

MainWidget::MainWidget(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWidgetClass())
{
    ui->setupUi(this);
}

MainWidget::~MainWidget()
{
    delete ui;
}
