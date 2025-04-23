#include "viewer.h"
#include "ui_viewer.h"

CViewer::CViewer(QWidget* parent_, const CGeneticAlgorithm* algorithm_) :
   QWidget(parent_), m_algorithm(algorithm_), ui(new Ui::Viewer())
{
   ui->setupUi(this);
   UpdateText();
   connect(ui->cbVariables, &QCheckBox::checkStateChanged, this, &CViewer::UpdateText);
   connect(ui->cbPredicates, &QCheckBox::checkStateChanged, this, &CViewer::UpdateText);
   connect(ui->cbIntegrityLimitation, &QCheckBox::checkStateChanged, this, &CViewer::UpdateText);
   connect(ui->cbGeneration, &QCheckBox::checkStateChanged, this, &CViewer::UpdateText);
   connect(ui->cbFitness, &QCheckBox::checkStateChanged, this, &CViewer::UpdateText);
   connect(ui->chTrueCondition, &QCheckBox::checkStateChanged, this, &CViewer::UpdateText);
   connect(ui->sbCountGeneration, &QSpinBox::valueChanged, this, &CViewer::UpdateText);
}

CViewer::~CViewer()
{
   delete ui;
}

void CViewer::SetAlgorithm(const CGeneticAlgorithm* algorithm)
{
   m_algorithm = algorithm;
}

void CViewer::UpdateText() const
{
   if (m_algorithm)
      ui->textBrowser->setText(m_algorithm->StringCustom(
         ui->cbVariables->isChecked(),
         ui->cbPredicates->isChecked(),
         ui->cbIntegrityLimitation->isChecked(),
         ui->cbGeneration->isChecked(),
         ui->cbFitness->isChecked(),
         ui->chTrueCondition->isChecked(),
         ui->sbCountGeneration->value()
      ));
}
