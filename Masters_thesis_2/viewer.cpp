#include "viewer.h"
#include "ui_viewer.h"

CViewer::CViewer(QWidget* parent_, const CGeneticAlgorithm* algorithm_) :
   QWidget(parent_), m_algorithm(algorithm_), ui(new Ui::Viewer())
{
   ui->setupUi(this);
   UpdateText();
   connect(ui->cbVariables, &QCheckBox::stateChanged, this, &CViewer::UpdateText);
   connect(ui->cbPredicates, &QCheckBox::stateChanged, this, &CViewer::UpdateText);
   connect(ui->cbIntegrityLimitation, &QCheckBox::stateChanged, this, &CViewer::UpdateText);
   connect(ui->cbGeneration, &QCheckBox::stateChanged, this, &CViewer::UpdateText);
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
         ui->sbCountGeneration->value()
      ));
}
