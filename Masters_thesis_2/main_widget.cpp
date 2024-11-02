#include "main_widget.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QTimer>

#include "viewer.h"

MainWidget::MainWidget(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWidgetClass())
{
    ui->setupUi(this);

    ui->progressBar->setVisible(false);

    connect(ui->pbLoad, &QPushButton::clicked, this, &MainWidget::onLoad);
    connect(ui->pbUpload, &QPushButton::clicked, this, &MainWidget::onUpload);
    connect(ui->pbStart, &QPushButton::clicked, this, &MainWidget::onStart);
    connect(ui->pbShowData, &QPushButton::clicked, this, &MainWidget::onShowData);
    connect(ui->cbSkipMutations, &QCheckBox::stateChanged, this, &MainWidget::onCheckStateChanged);
    connect(ui->sbIterations, &QSpinBox::valueChanged, this, &MainWidget::onSpinBoxValueChanged);

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
      if (!m_algorithm.FillDataInFile(path, strError))
         QMessageBox::warning(this, "Ошибка загрузки данных", strError);
   }
}

void MainWidget::onUpload()
{
   QString path = QFileDialog::getSaveFileName(this, "Сохранение результата", "", "Текстовые файлы (*.txt)");

   if (!path.isEmpty())
   {
      QString strError;
      if (!m_algorithm.WriteGenerationsInFile(path, strError))
         QMessageBox::warning(this, "Ошибка выгрузки данных", strError);
   }
}

void MainWidget::onStart()
{
   ui->pbStart->setEnabled(false);
   ui->progressBar->setVisible(true);
   ui->progressBar->setValue(0);

   QThread* thread = new QThread();
   m_algorithm.moveToThread(thread);

   connect(thread, &QThread::started, [&]()
      {
         m_algorithm.StartForThread(ui->sbIndivids->value(),
            ui->sbIterations->value(),
            ui->gbMutation->isChecked(),
            ui->sbMutation->value(),
            ui->cbSkipMutations->isChecked() ? ui->sbSkipMutations->value() : 0);
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
      m_dlgViewer = new CViewer(this);
      m_dlgViewer->setAttribute(Qt::WA_DeleteOnClose);
      m_dlgViewer->setWindowFlag(Qt::Window);

      connect(m_dlgViewer, &CViewer::destroyed, [&]() {m_dlgViewer = nullptr; });
   }

   QString strText, strError;
   bool successfully = false;

   successfully = m_algorithm.HasGenerations() ? m_algorithm.GetVarAndGen(strText, strError) : m_algorithm.GetVarAndCond(strText, strError);

   if (successfully)
      m_dlgViewer->SetText(strText);
   else
   {
      QMessageBox::warning(this, "Внимание", strError);
      return;
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

   if (pSender == ui->cbSkipMutations)
   {
      ui->sbSkipMutations->setEnabled(value_);
   }
}

void MainWidget::onSpinBoxValueChanged(int value_)
{
   QSpinBox* pSender = qobject_cast<QSpinBox*>(sender());
   if (!pSender)
      return;

   if (pSender == ui->sbIterations)
   {
      ui->sbSkipMutations->setMaximum(value_);
   }
}

void MainWidget::onUpdateProgress(int progress_)
{
   ui->progressBar->setValue(progress_);
}

void MainWidget::onShowError(const QString& messege_)
{
   QMessageBox::critical(this, "Ошибка", messege_);
}

void MainWidget::onEndingCalc()
{
   ui->pbStart->setEnabled(true);

   if (m_dlgViewer && ui->progressBar->value() == 100)
   {
      QString strText, strError;
      if (m_algorithm.GetVarAndGen(strText, strError))
         m_dlgViewer->SetText(strText);
   }

   // Скрытие прогресс бара через 5 секунд.
   //QTimer* timer = new QTimer;
   //timer->setSingleShot(true);
   //connect(timer, &QTimer::timeout, [&]() {ui->progressBar->setVisible(false); });
   //connect(timer, &QTimer::timeout, timer, &QObject::deleteLater);
   //timer->start(5000);
}
