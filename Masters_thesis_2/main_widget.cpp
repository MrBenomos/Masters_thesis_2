#include "main_widget.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QTimer>

#include "viewer.h"
#include "exception.h"

constexpr quint64 MS_IN_SEC = 1000;
constexpr quint64 MS_IN_MIN = MS_IN_SEC * 60;
constexpr quint64 MS_IN_HOUR = MS_IN_MIN * 60;

static QString GetStringRunTime(quint64 milleseconds_)
{
   quint32 seconds = milleseconds_ / MS_IN_SEC;
   quint32 minutes = milleseconds_ / MS_IN_MIN;
   quint16 hours = milleseconds_ / MS_IN_HOUR;

   QString sRunTime;
   if (hours > 0)
      sRunTime.append(QString::number(hours) + ':');
   if (minutes > 0)
      sRunTime.append(QString::number(minutes % 60) + ':');
   if (seconds > 0)
      sRunTime.append(QString::number(seconds % 60));
   else
      sRunTime.append('0');

   sRunTime.append('.');
   sRunTime.append(QString::number(milleseconds_ % 1000));

   if (minutes == 0)
      sRunTime.append(" c");

   return sRunTime;
}

MainWidget::MainWidget(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWidgetClass())
{
   qRegisterMetaType<CException>("CException");

    ui->setupUi(this);

    ui->progressBar->setVisible(false);
    ui->pbContinue->setVisible(false);

    connect(ui->pbLoad, &QPushButton::clicked, this, &MainWidget::onLoad);
    connect(ui->pbUpload, &QPushButton::clicked, this, &MainWidget::onUpload);
    connect(ui->pbStart, &QPushButton::clicked, this, &MainWidget::onStart);
    connect(ui->pbContinue, &QPushButton::clicked, this, &MainWidget::onContinue);
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
   if (ui->cbRunTime->isChecked())
   {
      quint64 avg = 0;
      if (m_vRunTime.size() > 0)
      {
         for (quint64 runTime : m_vRunTime)
            avg += runTime;

         avg /= m_vRunTime.size();
      }

      QMessageBox::information(this, tr("Время выполнения"),
         tr("Среднее время выполнения по результатам %1 запусков: %2.").arg(m_vRunTime.size()).arg(GetStringRunTime(avg)));
   }

   m_vRunTime.clear();

   QString path = QFileDialog::getOpenFileName(this, "Выберите файл для загрузки данных", "", "Текстовые файлы (*.txt)");
   if (!path.isEmpty())
   {
      QString strError;
      m_algorithm.FillDataInFile(path);

      ui->pbContinue->setVisible(false);

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
   runAlgorithm(false);
}

void MainWidget::onContinue()
{
   runAlgorithm(true);
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

void MainWidget::onEndingCalc(bool bSuccess_)
{
   if (m_timer.isValid())
   {
      m_vRunTime.push_back(m_timer.elapsed());
      m_timer.invalidate();

      if (ui->cbRunTime->isChecked())
         QMessageBox::information(this, tr("Время выполнения"), tr("Расчет занял: %1.").arg(GetStringRunTime(m_vRunTime.back())));
   }
   else
   {
      QMessageBox::warning(this, tr("Время выполнения"),
         tr("Не удалось вычислить время расчета: отсутствует отметка времени начала расчета."));
   }

   if (bSuccess_)
      ui->pbContinue->setVisible(true);

   ui->pbStart->setEnabled(true);
   ui->pbContinue->setEnabled(true);

   if (m_dlgViewer)
      m_dlgViewer->UpdateText();
}

void MainWidget::runAlgorithm(bool bContinue_)
{
   ui->pbStart->setEnabled(false);
   ui->pbContinue->setEnabled(false);
   ui->progressBar->setVisible(true);
   ui->progressBar->setValue(0);

   m_algorithm.SetCostAddingPredicate(ui->sbCostAdding->value());
   m_algorithm.SetLimitOfArgumentsChange(ui->sbCostArguments->value());

   QThread* thread = new QThread();
   m_algorithm.moveToThread(thread);

   connect(thread, &QThread::started, [this, bContinue_]()
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
            ui->sbMutationsIndivids->value(),
            bContinue_,
            ui->cbBoost->isChecked());
      }
   );
   connect(thread, &QThread::finished, thread, &QThread::deleteLater);
   connect(&m_algorithm, &CGeneticAlgorithm::signalEnd, thread, &QThread::quit);

   m_timer.start();
   thread->start();
}
