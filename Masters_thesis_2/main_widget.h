#pragma once

#include <QtWidgets/QMainWindow>
#include <QElapsedTimer>
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
   void onContinue();
   void onShowData();
   void onCheckStateChanged(int value_);
   void onIterationsChanged(int value_);

   void onUpdateProgress(int progress_);
   void onShowError(const CException& messege_);
   void onEndingCalc(bool bSuccess_);

private:

   void runAlgorithm(bool bContinue_);

private:
   CGeneticAlgorithm m_algorithm;
   QString m_sInput;
   QString m_sOutput;
   Ui::MainWidgetClass* ui;
   CViewer* m_dlgViewer = nullptr;
   QElapsedTimer m_timer;
   std::vector<quint64> m_vRunTime;
};
