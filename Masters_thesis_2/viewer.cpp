#include "viewer.h"
#include "ui_viewer.h"

CViewer::CViewer(QWidget* parent) :
   QWidget(parent), ui(new Ui::Viewer())
{
   ui->setupUi(this);
}

CViewer::~CViewer()
{
   delete ui;
}

void CViewer::SetText(const QString& text_)
{
   ui->textBrowser->setText(text_);
}
