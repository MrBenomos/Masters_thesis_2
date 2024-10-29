#pragma once
#include <QWidget>

namespace Ui { class Viewer; }

class CViewer : public QWidget
{
   Q_OBJECT

public:
   CViewer(QWidget* parent = nullptr);
   ~CViewer();

   void SetText(const QString& text_);

private:
   Ui::Viewer* ui = nullptr;
};