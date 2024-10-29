#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_main_widget.h"
#include "genetic_algorithm.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWidgetClass; }
QT_END_NAMESPACE

class CViewer;

class MainWidget : public QMainWindow
{
   Q_OBJECT

public:
   MainWidget(QWidget* parent = nullptr);
   ~MainWidget();

private slots:
   void onLoad();
   void onUpload();
   void onStart();
   void onShowData();

   void onUpdateProgress(int progress_);
   void onShowError(const QString& messege_);
   void onEndingCalc();

private:
   CGeneticAlgorithm m_algorithm;
   QString m_sInput;
   QString m_sOutput;
   Ui::MainWidgetClass* ui;
   CViewer* m_dlgViewer = nullptr;
};
