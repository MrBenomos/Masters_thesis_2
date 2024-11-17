#pragma once
#include <QWidget>
#include "genetic_algorithm.h"

namespace Ui { class Viewer; }

class CViewer : public QWidget
{
   Q_OBJECT

public:
   CViewer(QWidget* parent_ = nullptr, const CGeneticAlgorithm* algorithm_ = nullptr);
   ~CViewer();

   void SetAlgorithm(const CGeneticAlgorithm* algorithm);
   void UpdateText() const;

private:
   Ui::Viewer* ui = nullptr;
   const CGeneticAlgorithm* m_algorithm;
};