#include <algorithm>

#include <QFile>
#include <QTextStream>

#include "genetic_algorithm.h"
#include "exception.h"
#include "global.h"

#define SYMBOL_COMPLETION_CONDEITION ';'
#define SPLITTER "===================="

#define EXEPT(_exeption_)\
{\
Q_EMIT signalError(_exeption_);\
return;\
}

#define ERROR(_message_, _title_, _location_)\
{\
Q_EMIT signalError(CException(_message_, _title_, _location_));\
return;\
}

#define EXEPTSIGNAL(_exeption_)\
{\
Q_EMIT signalError(_exeption_);\
Q_EMIT signalEnd();\
return;\
}

#define ERRORSIGNAL(_message_, _title_, _location_)\
{\
Q_EMIT signalError(CException(_message_, _title_, _location_));\
Q_EMIT signalEnd();\
return;\
}

CGeneticAlgorithm::CGeneticAlgorithm()
{
   m_rand.UseNewNumbers();
}

void CGeneticAlgorithm::SetConditionsFromString(const QString& str_)
{
   qsizetype length = str_.length();
   try
   {
      for (qsizetype i = 0; i < length;)
      {
         bool bLeftEnd = false; // был символ конца левой части
         bool bRightEnd = false; // был символ конца правой части
         SCondition cond;

         // считываем левую часть условия
         while (!skipSpace(str_, i) && !bLeftEnd)
         {
            if (hasSymbol(str_, i, '-'))
            {
               if (hasSymbol(str_, i, '>'))
               {
                  bLeftEnd = true;
                  break; // выход из цикла по левой части условия
               }
               else
                  throw CException("После символа \'-\' ожидался символ \'>\'.");
            }

            cond.left.push_back(getPredicate(str_, i));
         }

         if (i < length) // есть что читать дальше
         {
            if (cond.left.empty())
               throw CException("Отсутствует левая часть условия.");
         }
         else
         {
            if (!cond.left.empty())
            {
               if (bLeftEnd)
                  throw CException("Отсутствует правая часть условия");
               else
                  throw CException("Незаконченная строка. Отсутствует \"->\" и правая часть условия.");
            }
            else if (bLeftEnd)
               throw CException("Встречено \"->\" в конце строки. Нет ни левой, ни правой части условия.");
            else
               break;
         }

         // считываем правую часть условия
         while (!skipSpace(str_, i))
         {
            if (hasSymbol(str_, i, ';'))
            {
               bRightEnd = true;
               break; // выход из цикла по правой части условия
            }

            cond.right.push_back(getPredicate(str_, i));
         }

         if (cond.right.empty())
            throw CException("Отсутствует правая часть условия.");

         if (!bRightEnd)
            throw CException(QString("Отсутствует символ завершения условия \'%1\'.").arg(SYMBOL_COMPLETION_CONDEITION));

         m_original.push_back(cond);
      }

      if (m_original.empty())
         throw CException("Нет ограничения целостности!");
   }
   catch (CException& error)
   {
      error.title("Ошибка добавления условия целостности");
      error.location("CGeneticAlgorithm::SetConditionFromString");
      throw error;
   }
}

void CGeneticAlgorithm::FillDataInFile(const QString& fileName_)
{
   QFile file(fileName_);
   if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
      ERROR(QString("Не удалось открыть файл :").arg(fileName_), "Ошибка загрузки данных", "CGeneticAlgorithm::FillDataInFile")

   Clear();

   QTextStream in(&file);
   QString str = in.readAll();

   qsizetype i = 0;

   try
   {
      m_predicates.SetVariables(highlightBlock(str, i));
      m_predicates.AddPredicates(highlightBlock(str, ++i));
      SetConditionsFromString(highlightBlock(str, ++i));
   }
   catch (CException& error)
   {
      Clear();
      QString title(error.title());
      title.append(". Ошибка загрузки данных");
      Q_EMIT signalError(error);
      return;
   }

   //SortRestriction(m_original);
}

QString CGeneticAlgorithm::StringVariables() const
{
   return m_predicates.StringVariables();
}

QString CGeneticAlgorithm::StringPredicates() const
{
   return m_predicates.StringPredicatesWithTable();
}

QString CGeneticAlgorithm::StringIntegrityLimitation() const
{
   return StringIntegrityLimitation(m_original);
}

QString CGeneticAlgorithm::StringGeneration(bool bFitness_, size_t count_) const
{
   count_ = qMin(count_, m_vGenerations.size());

   QString str;

   if (bFitness_)
   {
      for (size_t iGen = 0; iGen < count_; ++iGen)
      {
         str += QString("#%1 = %2%3").arg(iGen + 1).arg(FitnessFunction(m_vGenerations.at(iGen))).arg(NEW_LINE);
         str += StringIntegrityLimitation(m_vGenerations.at(iGen), true);
      }
   }
   else
   {
      for (size_t iGen = 0; iGen < count_; ++iGen)
         str += StringIntegrityLimitation(m_vGenerations.at(iGen), true);
   }

   str.chop(COUNT_SYMB_NEW_LINE);

   return str;
}

QString CGeneticAlgorithm::StringCustom(bool bVariables_, bool bPredicates_, bool bIntegrityLimitation_, bool bGeneration_, bool bFitness_, size_t countIndividuals_) const
{
   QString str;

   try
   {
      QTextStream out(&str);

      qsizetype oldSize = str.size();

      if (bVariables_)
         out << StringVariables();

      if (bPredicates_)
      {
         if (oldSize < str.size())
         {
            out << Qt::endl << Qt::endl;
            oldSize = str.size();
         }

         out << StringPredicates();
      }


      if (bIntegrityLimitation_)
      {
         if (oldSize < str.size())
         {
            out << Qt::endl << Qt::endl;
            oldSize = str.size();
         }

         if (str.size() != 0)
         {
            out << SPLITTER;
            out << Qt::endl << Qt::endl;
            oldSize = str.size();
         }

         out << StringIntegrityLimitation();
      }

      if (bGeneration_)
      {
         if (oldSize < str.size())
         {
            out << Qt::endl << Qt::endl;
            out << SPLITTER;
            out << Qt::endl << Qt::endl;
            oldSize = str.size();
         }
         else if (str.size() != 0 && !bIntegrityLimitation_)
         {
            out << SPLITTER;
            out << Qt::endl << Qt::endl;
            oldSize = str.size();
         }

         out << StringGeneration(bFitness_, countIndividuals_);
      }
   }
   catch (const CException& error)
   {
      Q_EMIT signalError(error);
   }

   return str;
}

void CGeneticAlgorithm::WriteInFile(const QString& fileName_, bool bVariables_, bool bPredicates_, bool bIntegrityLimitation_, bool bGeneration_, bool bFitness_, size_t countIndividuals_) const
{
   QFile file(fileName_);

   if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
      ERROR("Не удалось открыть файл: " + fileName_, "Ошибка выгрузки данных", "CGeneticAlgorithm::WriteInFile")

   try
   {
      file.write(StringCustom(bVariables_, bPredicates_, bIntegrityLimitation_, bGeneration_, bFitness_, countIndividuals_).toLocal8Bit());
   }
   catch (CException& error)
   {
      QString title(error.title());
      title.append(". Ошибка выгрузки данных");
      Q_EMIT signalError(error);
   }
}

void CGeneticAlgorithm::Start(int countIndividuals_, int countIterations_, double percentMutationArguments_, int countSkipMutationArg_, double percentMutationPredicates_, int countSkipMutationPred_, double percentIndividualsUndergoingMutation_)
{
   if (countIndividuals_ < 2)
      ERRORSIGNAL("Количество особей должно быть не меньше 2, для создания потомства.", "Ошибка запуска", "")

   if (countIterations_ < 0)
      ERRORSIGNAL("Не может быть отрицательного числа итераций!", "Ошибка запуска", "")

   if (countSkipMutationArg_ < 0)
      ERRORSIGNAL("Не может быть отрицательного пропуска мутаций аргументов!", "Ошибка запуска", "")

   if (countSkipMutationArg_ < 0)
      ERRORSIGNAL("Не может быть отрицательного пропуска мутаций предикатов!", "Ошибка запуска", "")

   if (percentMutationArguments_ < 0)
      percentMutationArguments_ = 0;

   if (percentMutationPredicates_ < 0)
      percentMutationPredicates_ = 0;

   if (percentIndividualsUndergoingMutation_ < 0)
      percentIndividualsUndergoingMutation_ = 0;

   // Сам запуск
   const double percentagePerIteration = 100. / countIterations_; // количество процентов за одну итерацию
   int percentageCompleted = 0;

   try
   {
      // Создание первого поколения
      CreateFirstGenerationRandom(countIndividuals_);

      for (size_t iGeneration = 0; iGeneration < countIterations_; ++iGeneration)
      {
         std::vector<TIntegrityLimitation> children;
         for (size_t iNewIndiv = 0; iNewIndiv < countIndividuals_ * 2; ++iNewIndiv)
         {
            // Селекция (выбор родителей) (турнирный отбор)
            size_t idxParent1, idxParent2;
            std::tie(idxParent1, idxParent2) = GetPairParents(countIndividuals_);

            // Скрещивание
            children.push_back(CrossingOnlyPredicates(m_vGenerations[idxParent1], m_vGenerations[idxParent2]));
         }

         // Мутации
         size_t countMutation = children.size() * percentIndividualsUndergoingMutation_ * 0.01;

         // Мутация аргументов
         if (percentMutationArguments_ > 0 && iGeneration < countIterations_ - countSkipMutationArg_)
            for (size_t iMutation = 0; iMutation < countMutation; ++iMutation)
            {
               MutationArguments(children.at(m_rand.Generate(0, children.size() - 1)), percentMutationArguments_ * 0.01);
            }

         // Мутация предикатов
         if (percentMutationPredicates_ > 0 && iGeneration < countIterations_ - countSkipMutationPred_)
            for (size_t iMutation = 0; iMutation < countMutation; ++iMutation)
            {
               MutationPredicates(children.at(m_rand.Generate(0, children.size() - 1)), percentMutationPredicates_ * 0.01);
            }

         // Селекция (полная замена, родителей "убиваем")
         Selection(children, countIndividuals_);

         // Отправляем сигнал о проценте выполнения.
         if (percentagePerIteration * iGeneration > percentageCompleted)
         {
            percentageCompleted = percentagePerIteration * iGeneration;
            Q_EMIT signalProgressUpdate(percentageCompleted);
         }
      }

      // Сортируем в порядке убывания
      SortGenerationDescendingOrder();
   }
   catch (const CException& error)
      EXEPTSIGNAL(error)

   Q_EMIT signalProgressUpdate(100);
   Q_EMIT signalEnd();

}

void CGeneticAlgorithm::Clear()
{
   m_predicates.Clear();
   m_original.clear();
   m_vGenerations.clear();
}

bool CGeneticAlgorithm::HasGenerations() const
{
   return !m_vGenerations.empty();
}

void CGeneticAlgorithm::SetLimitOfArgumentsChange(double from_)
{
   if (from_ <= 0 || from_ >= 1)
      ERROR("Невозможно установить цену нижней границы для измененных аргументов выходящую за интервал (0; 1).",
         "Некорректное значение", "CGeneticAlgorithm::SetRangeOfArgumentsChange")

   m_delta = 1 / from_;
   m_delta_1 = m_delta - 1;
}

void CGeneticAlgorithm::SetCostAddingPredicate(double cost_)
{
   if (cost_ < 0 || cost_ > 1)
      ERROR("Невозможно установить стоимость добавления предиката выходящую за диапазон [0; 1].",
         "Некорректное значение", "CGeneticAlgorithm::SetCostAddingPredicate")

   m_costAddingPredicate = cost_;
}

bool CGeneticAlgorithm::isIllegalSymbol(QChar symbol_)
{
   const QChar illegalSymbols[] = { ',', '-','>', '$', '(', ')', SYMBOL_COMPLETION_CONDEITION };

   for (const auto& illegal : illegalSymbols)
      if (symbol_ == illegal)
         return true;

   return false;
}

QString CGeneticAlgorithm::StringCondition(const SCondition& condition_) const
{
   QString str;

   for (const auto& leftPart : condition_.left)
      str += m_predicates.StringPredicateWithArg(leftPart.idxPredicate, leftPart.idxTable) + ' ';

   str.append("->");

   for (const auto& rightPart : condition_.right)
      str += ' ' + m_predicates.StringPredicateWithArg(rightPart.idxPredicate, rightPart.idxTable);

   return str.append(';');
}

QString CGeneticAlgorithm::StringIntegrityLimitation(const TIntegrityLimitation& integrityLimitation_, bool bInsertNewLine_) const
{
   QString str;
   for (const auto& condition : integrityLimitation_)
      str += StringCondition(condition) + NEW_LINE;

   if (!bInsertNewLine_)
      str.chop(COUNT_SYMB_NEW_LINE);
   else
      str.append(NEW_LINE);

   return str;
}

void CGeneticAlgorithm::CreateFirstGenerationRandom(size_t count_)
{
   if (count_ < 2)
      throw CException("Количество особей должно быть больше 1.", "Ошибка генерации первого поколения", "CGeneticAlgorithm::CreateFirstGenerationRandom");

   if (m_predicates.CountVariables() < 1)
      throw CException("Количество переменных должно быть не меньше 1.", "Ошибка генерации первого поколения", "CGeneticAlgorithm::CreateFirstGenerationRandom");

   if (m_predicates.IsEmpty())
      throw CException("Количество предикатов должно быть не меньше 1.", "Ошибка генерации первого поколения", "CGeneticAlgorithm::CreateFirstGenerationRandom");

   const size_t sizeOrigin = m_original.size();
   const size_t idxLastPredicates = m_predicates.CountPredicates() - 1;

   m_vGenerations.clear();

   for (size_t iGen = 0; iGen < count_; ++iGen)
   {
      TIntegrityLimitation conds(sizeOrigin);

      for (size_t iCond = 0; iCond < sizeOrigin; ++iCond)
      {
         // Не целесообразно делать условие сильно больше или меньше чем изначальное.
         // Будем брать в пределах двух. Не больше чем в 2 раза и не меньше чем в 2 раза.
         size_t sizeCond = m_rand.Generate(m_original.at(iCond).GeneralCount() / 2, m_original.at(iCond).GeneralCount() * 2);
         SCondition cond;

         for (size_t iPred = 0; iPred < sizeCond; ++iPred)
         {
            size_t idxPred = m_rand.Generate(0, idxLastPredicates);
            size_t idxArg = m_rand.Generate(0, m_predicates.CountArguments(idxPred));
            SPredicateInstance predInst(idxPred, idxArg);

            if (m_rand.Generate(0, 1))
               cond.left.push_back(std::move(predInst));
            else
               cond.right.push_back(std::move(predInst));
         }

         conds[iCond] = cond;
      }

      //SortRestriction(conds);
      m_vGenerations.push_back(conds);
   }
}

CGeneticAlgorithm::TIntegrityLimitation CGeneticAlgorithm::CrossingOnlyPredicates(const TIntegrityLimitation& parent1_, const TIntegrityLimitation& parent2_) const
{
   if (parent1_.size() != parent2_.size())
      throw CException("Разное количество условий целостности у родителей!", "Ошибка скрещивания", "CGeneticAlgorithm::CrossingOnlyPredicates");

   TIntegrityLimitation child;

   for (size_t iCond = 0; iCond < parent1_.size(); ++iCond)
   {
      const SCondition& cond1 = parent1_.at(iCond);
      const SCondition& cond2 = parent2_.at(iCond);

      SCondition newCond;

      // левая часть
      size_t minPred = qMin(cond1.left.size(), cond2.left.size());
      for (size_t iPred = 0; iPred < minPred; ++iPred)
      { 
         if (m_rand.Generate(0, 1))
            newCond.left.push_back(cond2.left.at(iPred));
         else
            newCond.left.push_back(cond1.left.at(iPred));
      }

      const SCondition& condL = cond1.left.size() == minPred ? cond2 : cond1;

      for (size_t iPred = minPred; iPred < condL.left.size(); ++iPred)
      {
         if (m_rand.Generate(0, 1))
            newCond.left.push_back(condL.left.at(iPred));
      }

      // правая часть
      minPred = qMin(cond1.right.size(), cond2.right.size());
      for (size_t iPred = 0; iPred < minPred; ++iPred)
      {
         if (m_rand.Generate(0, 1))
            newCond.right.push_back(cond2.right.at(iPred));
         else
            newCond.right.push_back(cond1.right.at(iPred));
      }

      const SCondition& condR = cond1.right.size() == minPred ? cond2 : cond1;

      for (size_t iPred = minPred; iPred < condR.right.size(); ++iPred)
      {
         if (m_rand.Generate(0, 1))
            newCond.right.push_back(condR.right.at(iPred));
      }

      child.push_back(newCond);
   }

   //SortRestriction(child);

   return child;
}

void CGeneticAlgorithm::Selection(const std::vector<TIntegrityLimitation>& individual_, size_t countSurvivors_)
{
   if (individual_.size() < countSurvivors_)
      throw CException("Количество выживших не должно быть меньше самих особей", "Ошибка селекции", "CGeneticAlgorithm::Selection");

   std::multimap<double, size_t, std::greater<double>> mapIdx;

   for (size_t iIndiv = 0; iIndiv < individual_.size(); ++iIndiv)
      mapIdx.emplace(FitnessFunction(individual_[iIndiv]), iIndiv);

   m_vGenerations.clear();

   auto itIdx = mapIdx.begin();

   for (size_t i = 0; i < countSurvivors_; ++i)
      m_vGenerations.push_back(individual_[(itIdx++)->second]);
}

void CGeneticAlgorithm::MutationArguments(TIntegrityLimitation& individual_, double ratio_) const
{
   if (ratio_ <= 0.)
      return;

   size_t countAllArg = 0;
   for (const auto& cond : individual_)
   {
      for (const auto& predInst : cond.left)
         countAllArg += m_predicates.CountArguments(predInst.idxPredicate);

      for (const auto& predInst : cond.right)
         countAllArg += m_predicates.CountArguments(predInst.idxPredicate);
   }

   const size_t iLastConditions = individual_.size() - 1;
   const size_t countVariables = m_predicates.CountVariables();

   const size_t countMutations = qMax(static_cast<size_t>(ratio_ * countAllArg), static_cast<size_t>(1));
   for (size_t i = 0; i < countMutations; ++i)
   {
      auto& fullCond = individual_.at(m_rand.Generate(0, iLastConditions)); // выбор условия
      auto& partCond = m_rand.Generate(0, 1) ? fullCond.right : fullCond.left; // выбор части условия (правая или левая)
      if (partCond.size() < 1)
         continue;

      auto& predInst = partCond.at(m_rand.Generate(0, partCond.size() - 1)); // выбор конкретного предиката в условии целостности
      size_t posArg = m_rand.Generate(0, m_predicates.CountArguments(predInst.idxPredicate) - 1); // выбор позиции аргумента у выбранного предиката
      size_t newValueVar = m_rand.Generate(0, countVariables - 1); // новое значение выбранного аргумента

      SPredicate realPredicate = m_predicates.GetPredicate(predInst.idxPredicate);
      auto realArguments = realPredicate.GetArgs(countVariables, predInst.idxTable);
      if (realArguments.empty())
         throw CException("Ошибка при мутации аргументов в одном из условий целостности.", "Непредвиденная ошибка", "CGeneticAlgorithm::MutationArguments");

      realArguments[posArg] = newValueVar;
      predInst.idxTable = realPredicate.GetIndex(countVariables, realArguments);
   }
}

void CGeneticAlgorithm::MutationPredicates(TIntegrityLimitation& individual_, double ratio_) const
{
   if (ratio_ <= 0.)
      return;

   size_t countPredicats = CountAllPredicates(individual_);

   const size_t iLastConditions = individual_.size() - 1;
   const size_t iLastPredicates = m_predicates.CountPredicates() - 1;

   const size_t countMutations = qMax(static_cast<size_t>(ratio_ * countPredicats), static_cast<size_t>(1));
   for (size_t i = 0; i < countMutations; ++i)
   {
      auto& fullCond = individual_.at(m_rand.Generate(0, iLastConditions));
      auto& partCond = m_rand.Generate(0, 1) ? fullCond.right : fullCond.left;
      if (partCond.size() < 1)
         continue;

      auto& predInst = partCond.at(m_rand.Generate(0, partCond.size() - 1));

      size_t newIdxPred = m_rand.Generate(0, iLastPredicates);
      size_t newIdxTable = m_rand.Generate(0, m_predicates.CountArguments(newIdxPred) - 1);

      predInst.idxPredicate = newIdxPred;
      predInst.idxTable = newIdxTable;
   }
}

size_t CGeneticAlgorithm::CountAllPredicates(const TIntegrityLimitation& individual_) const
{
   size_t count = 0;
   for (const auto& cond : individual_)
      count += cond.left.size() + cond.right.size();

   return count;
}

bool CGeneticAlgorithm::IsCorrectCondition(const SCondition& Cond_) const
{
   return !(Cond_.left.empty() || Cond_.right.empty());
}

bool CGeneticAlgorithm::IsTrueCondition(const SCondition& Cond_) const
{
   // Если в левой части 0, то импликация всегда истинна. (0->X = 1)
   // Если в правой части 1, то импликация тоже всегда истинна. (X->1 = 1)

   for (const auto& predInst : Cond_.left)
   {
      if (!m_predicates.GetValuePredicate(predInst.idxPredicate, predInst.idxTable))
         return true;
   }

   for (const auto& predInst : Cond_.right)
   {
      if (m_predicates.GetValuePredicate(predInst.idxPredicate, predInst.idxTable))
         return true;
   }

   return false;
}

double CGeneticAlgorithm::FitnessFunction(const TIntegrityLimitation& conds_) const
{
   if (m_original.size() != conds_.size())
      throw CException("Попытка фитнеса двух разных ограничений целостности. Обратитесь к разработчику.", "Непредвиденная ошибка.", "CGeneticAlgorithm::FitnessFunction");

   double fitnes = 0;

   for (size_t iCond = 0; iCond < m_original.size(); ++iCond)
   {
      if (!IsCorrectCondition(conds_.at(iCond)))
      {
         fitnes += -1. / m_original.size();
         continue;
      }

      SCounts count;

      count += quantitativeAssessment(m_original.at(iCond).left, conds_.at(iCond).left);
      count += quantitativeAssessment(m_original.at(iCond).right, conds_.at(iCond).right);

      const double dMultiplierArgs = getMultiplierArguments(count.diffArg, count.totalArg);
      double fitnesCond = IsTrueCondition(conds_.at(iCond)) ? 0. : -1.;
      fitnesCond += dMultiplierArgs * count.matchPred;
      fitnesCond += m_costAddingPredicate * count.addedPred;
      fitnesCond /= count.matchPred + count.addedPred + count.deletedPred;

      fitnes += fitnesCond / m_original.size();
   }

   return fitnes;
}

std::multimap<int, size_t> CGeneticAlgorithm::findMinDifference(const SPredicateInstance& sample_, const TPartCondition& verifiable_) const
{
   // Число отличий, индекс в векторе условия.
   std::multimap<int, size_t> result;

   try
   {
      const size_t countVariables = m_predicates.CountVariables();
      const SPredicate& samplePredicate = m_predicates.GetPredicate(sample_.idxPredicate);
      const std::vector<size_t>& sampleArgs = samplePredicate.GetArgs(countVariables, sample_.idxTable);

      if (sampleArgs.empty())
         throw CException("", "Ошибка. Обратитесь к разработчику", "CGeneticAlgorithm::findMinDifference");

      for (size_t iPredInst = 0; iPredInst < verifiable_.size(); ++iPredInst)
      {
         const SPredicateInstance& predInst = verifiable_.at(iPredInst);
         if (predInst.idxPredicate != sample_.idxPredicate)
            continue;

         const SPredicate& verPredicate = m_predicates.GetPredicate(predInst.idxPredicate);
         const std::vector<size_t>& verArgs = verPredicate.GetArgs(countVariables, predInst.idxTable);

         if (verArgs.empty() || verArgs.size() != sampleArgs.size())
            throw CException("", "Ошибка. Обратитесь к разработчику", "CGeneticAlgorithm::findMinDifference");

         int counter = 0;
         for (size_t i = 0; i < verArgs.size(); ++i)
         {
            if (sampleArgs[i] != verArgs[i])
               ++counter;
         }

         result.emplace(counter, iPredInst);
      }
   }
   catch (CException& error)
   {
      if (!strlen(error.message()))
         error.addToBeginningMessage("Ошибка при нахождении предикатов с минимальным отличием.");
      else
         error.message("Ошибка при нахождении предикатов с минимальным отличием.");

      throw error;
   }

   return result;
}

CGeneticAlgorithm::SCounts CGeneticAlgorithm::quantitativeAssessment(const TPartCondition& sample_, const TPartCondition& verifiable_) const
{
   SCounts count;

   std::vector<bool> vUsedPredInst(verifiable_.size(), false);

   for (size_t iPredInst = 0; iPredInst < sample_.size(); ++iPredInst)
   {
      auto mapIdentical = findMinDifference(sample_.at(iPredInst), verifiable_);

      if (mapIdentical.empty())
      {
         ++count.deletedPred;
      }
      else
      {
         bool bFoundPair = false;
         for (const auto& candidate : mapIdentical)
         {
            const size_t idxInCond = candidate.second;
            if (!vUsedPredInst.at(idxInCond))
            {
               vUsedPredInst.at(idxInCond) = true;
               count.diffArg += candidate.first;
               count.totalArg += m_predicates.CountArguments(verifiable_.at(idxInCond).idxPredicate);
               bFoundPair = true;
               break;
            }
         }

         if (bFoundPair)
            count.matchPred += 2; // т.к. совпадают у обоих.
         else
            ++count.deletedPred;
      }
   }

   for (bool used : vUsedPredInst)
   {
      if (!used)
         ++count.addedPred;
   }

   return count;
}

double CGeneticAlgorithm::getMultiplierArguments(size_t differences_, size_t total_) const
{
   if (m_delta < 1)
      throw CException("Ошибка задания дельты.", "Некорректно задан параметр.", "CGeneticAlgorithm::getMultiplierArguments"); // +++++ Перенести в место задания этой переменной и поменять текст.

   return 1 - (differences_ * m_delta_1) / (total_ * m_delta);
}

size_t CGeneticAlgorithm::SelectRandParent(size_t countIndividuals_) const
{
   if (countIndividuals_ < 2)
      throw CException("Слишком мало индивидуумов!", "Ошибка выбора родителя", "CGeneticAlgorithm::SelectRandParent");

   m_rand.SetBoundaries(0, countIndividuals_ - 1);

   qint64 first = m_rand.Generate();
   qint64 second = m_rand.Generate();
   while (first == second)
      second = m_rand.Generate();

   return FitnessFunction(m_vGenerations[first]) < FitnessFunction(m_vGenerations[second]) ? second : first;
}

std::pair<size_t, size_t> CGeneticAlgorithm::GetPairParents(size_t countIndividuals_) const
{
   size_t index1 = SelectRandParent(countIndividuals_);
   size_t index2 = SelectRandParent(countIndividuals_);

   while (index1 == index2)
      index2 = SelectRandParent(countIndividuals_);

   return std::make_pair(index1, index2);
}

void CGeneticAlgorithm::SortGenerationDescendingOrder()
{
   std::multimap<double, TIntegrityLimitation, std::greater<double>> mapIdx;

   for (const auto& individ : m_vGenerations)
      mapIdx.emplace(FitnessFunction(individ), individ);

   m_vGenerations.clear();

   for (const auto& individ : mapIdx)
      m_vGenerations.push_back(individ.second);

}

QString CGeneticAlgorithm::highlightBlock(const QString& str_, qsizetype& index_)
{
   qsizetype start = index_;

   for (; index_ < str_.size(); ++index_)
   {
      if (str_.at(index_) == '$')
         break;
   }

   return str_.mid(start, index_ - start);
}

QString CGeneticAlgorithm::highlightName(const QString& str_, qsizetype& index_)
{
   qsizetype start = index_;

   for (; index_ < str_.size(); ++index_)
   {
      const QChar& symb = str_.at(index_);
      if (symb.isSpace() || isIllegalSymbol(symb))
         break;
   }

   return str_.mid(start, index_ - start);
}

bool CGeneticAlgorithm::hasSymbol(const QString str_, qsizetype& index_, QChar symbol_)
{
   if (index_ < str_.size() && str_.at(index_) == symbol_)
   {
      ++index_;
      return true;
   }

   return false;
}

CGeneticAlgorithm::SPredicateInstance CGeneticAlgorithm::getPredicate(const QString& str_, qsizetype& index_) const
{
   QString predicateName = highlightName(str_, index_);
   if (predicateName.isEmpty())
      throw CException("Некорректное имя предиката!");

   size_t idxPredicate = m_predicates.GetIndexPredicate(predicateName);
   if (idxPredicate == SIZE_MAX)
      throw CException(QString("Не найден предикат с именем \"%1\"").arg(predicateName));

   skipSpace(str_, index_);
   if (!hasSymbol(str_, index_, '('))
      throw CException(QString("После имени предиката \"%1\" ожидаось \'(\'.").arg(predicateName));

   size_t countArg = m_predicates.CountArguments(idxPredicate);
   std::vector<QString> vVariables(countArg);
   for (quint16 iArg = 0; iArg < countArg; ++iArg)
   {
      if (skipSpace(str_, index_))
         throw CException(QString("Недостаточно аргументов у предиката \"%1\".").arg(predicateName));

      vVariables[iArg] = highlightName(str_, index_);
      if (vVariables.at(iArg).isEmpty())
         throw CException(QString("Некорректное имя переменной у аргумента предиката \"%1\"").arg(predicateName));

      skipSpace(str_, index_, ',');
   }

   if (!hasSymbol(str_, index_, ')'))
      throw CException(QString("Ожидалось \')\' у предиката \"%1\".").arg(predicateName));

   size_t idxArguments = m_predicates.GetIndexArgument(idxPredicate, vVariables);
   if (idxArguments == SIZE_MAX)
      throw CException(QString("Отсутствует значение для полученного набора аргументов у предиката \"%1\"").arg(predicateName));

   return SPredicateInstance(idxPredicate, idxArguments);
}

CGeneticAlgorithm::SCounts& CGeneticAlgorithm::SCounts::operator+=(const SCounts& added_)
{
   diffArg += added_.diffArg;
   totalArg += added_.totalArg;

   matchPred += added_.matchPred;
   addedPred += added_.addedPred;
   deletedPred += added_.deletedPred;

   return *this;
}