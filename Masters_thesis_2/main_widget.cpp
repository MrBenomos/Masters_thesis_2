#include "main_widget.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QTimer>

#include "viewer.h"
#include "exception.h"

MainWidget::MainWidget(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWidgetClass())
{
   qRegisterMetaType<CException>("CException");

    ui->setupUi(this);

    ui->progressBar->setVisible(false);

    connect(ui->pbLoad, &QPushButton::clicked, this, &MainWidget::onLoad);
    connect(ui->pbUpload, &QPushButton::clicked, this, &MainWidget::onUpload);
    connect(ui->pbStart, &QPushButton::clicked, this, &MainWidget::onStart);
    connect(ui->pbShowData, &QPushButton::clicked, this, &MainWidget::onShowData);
    connect(ui->cbSkipMutationsPredicates, &QCheckBox::checkStateChanged, this, &MainWidget::onCheckStateChanged);
    connect(ui->cbSkipMutationsArgumetns, &QCheckBox::checkStateChanged, this, &MainWidget::onCheckStateChanged);
    connect(ui->sbIterations, &QSpinBox::valueChanged, this, &MainWidget::onIterationsChanged);

    connect(&m_algorithm, &CGeneticAlgorithm::signalProgressUpdate, this, &MainWidget::onUpdateProgress);
    connect(&m_algorithm, &CGeneticAlgorithm::signalError, this, &MainWidget::onShowError);
    connect(&m_algorithm, &CGeneticAlgorithm::signalEnd, this, &MainWidget::onEndingCalc);
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::onLoad()
{
   QString path = QFileDialog::getOpenFileName(this, "Выберите файл для загрузки данных", "", "Текстовые файлы (*.txt)");
   if (!path.isEmpty())
   {
      QString strError;
      m_algorithm.FillDataInFile(path);

      if (m_dlgViewer)
         m_dlgViewer->UpdateText();
   }
}

void MainWidget::onUpload()
{
   QString path = QFileDialog::getSaveFileName(this, "Сохранение результата", "", "Текстовые файлы (*.txt)");

   if (!path.isEmpty())
   {
      QString strError;
      m_algorithm.WriteInFile(path);
   }
}

void MainWidget::onStart()
{
   ui->pbStart->setEnabled(false);
   ui->progressBar->setVisible(true);
   ui->progressBar->setValue(0);

   m_algorithm.SetCostAddingPredicate(ui->sbCostAdding->value());
   m_algorithm.SetLimitOfArgumentsChange(ui->sbCostArguments->value());

   QThread* thread = new QThread();
   m_algorithm.moveToThread(thread);

   connect(thread, &QThread::started, [this]()
      {
         bool bMutationArg = ui->gbMutationArguments->isChecked();
         bool bSkipMutationArg = ui->cbSkipMutationsArgumetns->isChecked();
         bool bMutationPred = ui->gbMutationPredicates->isChecked();
         bool bSkipMutationPred = ui->cbSkipMutationsPredicates->isChecked();
         m_algorithm.Start(ui->sbIndivids->value(),
            ui->sbIterations->value(),
            bMutationArg ? ui->sbMutationsArguments->value() : 0,
            bSkipMutationArg ? ui->sbSkipMutationsArguments->value() : 0,
            bMutationPred ? ui->sbMutationsPredicates->value() : 0,
            bSkipMutationPred ? ui->sbSkipMutationsPredicates->value() : 0,
            ui->sbMutationsIndivids->value());
      }
   );
   connect(thread, &QThread::finished, thread, &QThread::deleteLater);
   connect(&m_algorithm, &CGeneticAlgorithm::signalEnd, thread, &QThread::quit);

   thread->start();
}

void MainWidget::onShowData()
{
   if (!m_dlgViewer)
   {
      m_dlgViewer = new CViewer(this, &m_algorithm);
      m_dlgViewer->setAttribute(Qt::WA_DeleteOnClose);
      m_dlgViewer->setWindowFlag(Qt::Window);

      connect(m_dlgViewer, &CViewer::destroyed, [&]() {m_dlgViewer = nullptr; });
   }

   m_dlgViewer->show();

   if (m_dlgViewer->isMinimized())
      m_dlgViewer->showNormal();
}

void MainWidget::onCheckStateChanged(int value_)
{
   QCheckBox* pSender = qobject_cast<QCheckBox*>(sender());
   if (!pSender)
      return;

   if (pSender == ui->cbSkipMutationsPredicates)
      ui->sbSkipMutationsPredicates->setEnabled(value_);
   else if (pSender == ui->cbSkipMutationsArgumetns)
      ui->sbSkipMutationsArguments->setEnabled(value_);
}

void MainWidget::onIterationsChanged(int value_)
{
   ui->sbSkipMutationsPredicates->setMaximum(value_);
   ui->sbSkipMutationsArguments->setMaximum(value_);
}

void MainWidget::onUpdateProgress(int progress_)
{
   ui->progressBar->setValue(progress_);
}

void MainWidget::onShowError(const CException& messege_)
{
   QMessageBox::critical(this, messege_.title(), messege_.what());
}

void MainWidget::onEndingCalc()
{
   ui->pbStart->setEnabled(true);

   if (m_dlgViewer)
      m_dlgViewer->UpdateText();
}
