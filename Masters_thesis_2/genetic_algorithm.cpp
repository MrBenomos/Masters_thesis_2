#include <algorithm>
#include <memory>
#include <cmath>

#include <QFile>
#include <QTextStream>

#include "genetic_algorithm.h"
#include "exception.h"
#include "global.h"
#include "counter.h"
#include "custom_math.h"

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
Q_EMIT signalEnd(false);\
return;\
}

#define ERRORSIGNAL(_message_, _title_, _location_)\
{\
Q_EMIT signalError(CException(_message_, _title_, _location_));\
Q_EMIT signalEnd(false);\
return;\
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= Статические функции =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// Возвращает количество всех аргументов во всем условии.
static int countAllArgumentsInCondition(const SCondition& condition_)
{
   int result = 0;

   condition_.ForEachPredicate([&result](const SPredicateTemplate& predTempl)
      {
         result += static_cast<int>(predTempl.arguments.size());
      });

   return result;
}

// Приводит аргументы к нормальному виду.
// Аргументы становятся в порядке возрастания от 0 до максимального отличного.
// Аргументы ктороые равны -1 остаются неизменными.
static void bringArgumentsBackToNormal(SCondition& condition_)
{
   std::unordered_map<int, int> replace;
   replace.emplace(-1, -1);
   int countDiffArg = 0;
   condition_.ForEachArgument([&replace, &countDiffArg](int& arg)
      {
         if (replace.emplace(arg, countDiffArg).second)
            ++countDiffArg;
      });

   condition_.ForEachArgument([&replace](int& arg)
      {
         arg = replace.at(arg);
      });

   condition_.maxArgument = countDiffArg - 1;
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= Методы класса =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

CGeneticAlgorithm::CGeneticAlgorithm()
{
   m_rand.UseNewNumbers();
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
      m_storage.SetVariables(highlightBlock(str, i));
      m_storage.AddPredicates(highlightBlock(str, ++i));
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
}

QString CGeneticAlgorithm::StringVariables() const
{
   return m_storage.StringVariables();
}

QString CGeneticAlgorithm::StringPredicates() const
{
   return m_storage.StringPredicatesWithTable();
}

QString CGeneticAlgorithm::StringIntegrityLimitation(bool bTrueCondition_, bool bUsefulnessCondition_) const
{
   return StringIntegrityLimitation(m_original, false, bTrueCondition_, bUsefulnessCondition_);
}

QString CGeneticAlgorithm::StringGeneration(bool bFitness_, bool bTrueCondition_, bool bUsefulnessCondition_, size_t count_) const
{
   count_ = qMin(count_, m_generation.size());

   QString str;

   TGeneration generation = m_generation;
   SortGenerationDescendingOrder(generation);

   if (bFitness_)
   {
      for (size_t iGen = 0; iGen < count_; ++iGen)
      {
         const TIntLimAndFitness& conds = generation.at(iGen);
         double valFitness = conds.second == -999. ? FitnessFunction(conds.first) : conds.second;

         str += QString("#%1 = %2%3").arg(iGen + 1).arg(valFitness).arg(NEW_LINE);
         str += StringIntegrityLimitation(conds.first, true, bTrueCondition_, bUsefulnessCondition_);
      }
   }
   else
   {
      for (size_t iGen = 0; iGen < count_; ++iGen)
         str += StringIntegrityLimitation(generation.at(iGen).first, true, bTrueCondition_, bUsefulnessCondition_);
   }

   str.chop(COUNT_SYMB_NEW_LINE);

   return str;
}

QString CGeneticAlgorithm::StringCustom(bool bVariables_, bool bPredicates_, bool bIntegrityLimitation_, bool bGeneration_, bool bFitness_, bool bTrueCondition_, bool bUsefulnessCondition_, size_t countIndividuals_) const
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

         out << StringIntegrityLimitation(bTrueCondition_, bUsefulnessCondition_);
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

         out << StringGeneration(bFitness_, bTrueCondition_, countIndividuals_);
      }
   }
   catch (const CException& error)
   {
      Q_EMIT signalError(error);
   }

   return str;
}

void CGeneticAlgorithm::WriteInFile(const QString& fileName_, bool bVariables_, bool bPredicates_, bool bIntegrityLimitation_, bool bGeneration_, bool bFitness_, bool bTrueCondition_, bool bUsefulnessCondition_, size_t countIndividuals_) const
{
   QFile file(fileName_);

   if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
      ERROR("Не удалось открыть файл: " + fileName_, "Ошибка выгрузки данных", "CGeneticAlgorithm::WriteInFile")

   try
   {
      file.write(StringCustom(bVariables_, bPredicates_, bIntegrityLimitation_, bGeneration_, bFitness_, bTrueCondition_, bUsefulnessCondition_, countIndividuals_).toLocal8Bit());
   }
   catch (CException& error)
   {
      QString title(error.title());
      title.append(". Ошибка выгрузки данных");
      Q_EMIT signalError(error);
   }
}

void CGeneticAlgorithm::Start(int countIndividuals_, int countIterations_, double percentMutationArguments_, int countSkipMutationArg_, double percentMutationPredicates_, int countSkipMutationPred_, double percentIndividualsUndergoingMutation_, bool bContinue_)
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
      if (!bContinue_ || m_generation.empty())
      {
         // Создание первого поколения
         CreateFirstGenerationRandom(countIndividuals_);
      }

      for (size_t iGeneration = 0; iGeneration < countIterations_; ++iGeneration)
      {
         TGeneration children(countIndividuals_ * 2);
         for (size_t iNewIndiv = 0; iNewIndiv < children.size(); ++iNewIndiv)
         {
            // Селекция (выбор родителей) (турнирный отбор)
            // На этом этапе фитнес должен быть известен.
            auto [idxParent1, idxParent2] = GetPairParents();

            // Скрещивание (нет смысла считать фитнес, все еще может поменяться).
            children[iNewIndiv] = std::make_pair(CrossingOnlyPredicates(m_generation[idxParent1].first, m_generation[idxParent2].first), -999.);
         }

         // Мутации
         size_t countMutation = children.size() * percentIndividualsUndergoingMutation_ * 0.01;

         // Мутация аргументов
         if (percentMutationArguments_ > 0 && iGeneration < countIterations_ - countSkipMutationArg_)
            for (size_t iMutation = 0; iMutation < countMutation; ++iMutation)
               MutationArguments(children.at(m_rand.Generate(0, children.size() - 1)).first, percentMutationArguments_ * 0.01);

         // Мутация предикатов
         if (percentMutationPredicates_ > 0 && iGeneration < countIterations_ - countSkipMutationPred_)
            for (size_t iMutation = 0; iMutation < countMutation; ++iMutation)
               MutationPredicates(children.at(m_rand.Generate(0, children.size() - 1)).first, percentMutationPredicates_ * 0.01);

         // Теперь надо посчитать фитнес.
         for (auto& individual : children)
            individual.second = FitnessFunction(individual.first);

         // Селекция (полная замена, родителей "убиваем")
         Selection(std::move(children), countIndividuals_);

         // Отправляем сигнал о проценте выполнения.
         if (percentagePerIteration * iGeneration > percentageCompleted)
         {
            percentageCompleted = percentagePerIteration * iGeneration;
            Q_EMIT signalProgressUpdate(percentageCompleted);
         }
      }

      // Сортируем в порядке убывания
      SortGenerationDescendingOrder(m_generation);
   }
   catch (const CException& error)
      EXEPTSIGNAL(error)

   Q_EMIT signalProgressUpdate(100);
   Q_EMIT signalEnd(true);
}

void CGeneticAlgorithm::Clear()
{
   m_storage.Clear();
   m_original.clear();
   m_generation.clear();
}

bool CGeneticAlgorithm::HasGenerations() const
{
   return !m_generation.empty();
}

void CGeneticAlgorithm::SetLimitOfArgumentsChange(double value_)
{
   if (value_ <= 0 || value_ >= 1)
      ERROR("Невозможно установить цену нижней границы для измененных аргументов выходящую за интервал (0; 1).",
         "Некорректное значение", "CGeneticAlgorithm::SetRangeOfArgumentsChange")

   m_minCostForArgDif = value_;
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
   const QChar illegalSymbols[] = { ',', '-','>', '$', '(', ')', '~', SYMBOL_COMPLETION_CONDEITION};

   for (const auto& illegal : illegalSymbols)
      if (symbol_ == illegal)
         return true;

   return false;
}

void CGeneticAlgorithm::SetConditionsFromString(const QString& str_)
{
   qsizetype length = str_.length();
   try
   {
      CParserTemplatePredicates parser(&m_storage);
      for (const QString& condition : str_.split(SYMBOL_COMPLETION_CONDEITION))
         if (!condition.isEmpty())
            m_original.push_back(parser.Parse(condition));

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

QString CGeneticAlgorithm::StringCondition(const SCondition& condition_) const
{
   CParserTemplatePredicates parser(&m_storage);
   QString str;

   for (const auto& predTempl : condition_.left)
      str += parser.GetStringTemplatePredicate(predTempl) + ' ';

   str.append("->");

   for (const auto& predTempl : condition_.right)
      str += ' ' + parser.GetStringTemplatePredicate(predTempl);

   return str.append(';');
}

QString CGeneticAlgorithm::StringIntegrityLimitation(const TIntegrityLimitation& integrityLimitation_, bool bInsertNewLine_, bool bTrueCondition_, bool bUsefulnessCondition_) const
{
   QString str;
   for (const auto& condition : integrityLimitation_)
   {
      str += StringCondition(condition);
      if (bTrueCondition_ || bUsefulnessCondition_)
      {
         std::pair<bool, bool> correctCondition = IsTrueCondition(condition);
         QString sTrueCondition = correctCondition.first ? "true" : "false";
         QString sUsefulnessCondition = correctCondition.second ? "true" : "false";

         str += " (";
         if (bTrueCondition_)
         {
            str += sTrueCondition;

            if (bUsefulnessCondition_)
               str += ", " + sUsefulnessCondition;
         }
         else
         {
            str += sUsefulnessCondition;
         }

         str += ')';

      }
      str += NEW_LINE;
   }

   if (!bInsertNewLine_)
      str.chop(COUNT_SYMB_NEW_LINE);
   else
      str.append(NEW_LINE);

   return str;
}

void CGeneticAlgorithm::FillRandomArguments(std::vector<int>& vArguments_, int& maxArg_) const
{
   for (int& arg : vArguments_)
   {
      arg = m_rand.Generate(0, maxArg_ + 2) - 1;
      if (arg == maxArg_ + 1)
         ++maxArg_;
   }
}

void CGeneticAlgorithm::CreateFirstGenerationRandom(size_t count_)
{
   if (count_ < 2)
      throw CException("Количество особей должно быть больше 1.", "Ошибка генерации первого поколения", "CGeneticAlgorithm::CreateFirstGenerationRandom");

   if (m_storage.CountVariables() < 1)
      throw CException("Количество переменных должно быть не меньше 1.", "Ошибка генерации первого поколения", "CGeneticAlgorithm::CreateFirstGenerationRandom");

   if (m_storage.IsEmpty())
      throw CException("Количество предикатов должно быть не меньше 1.", "Ошибка генерации первого поколения", "CGeneticAlgorithm::CreateFirstGenerationRandom");

   const size_t sizeOrigin = m_original.size(); // количество условий в изначальном ограничении целостности
   const size_t idxLastPredicate = m_storage.CountPredicates() - 1; // индекс последнего предиката

   m_generation.clear();

   for (size_t iGen = 0; iGen < count_; ++iGen)
   {
      TIntegrityLimitation conds(sizeOrigin);

      for (size_t iCond = 0; iCond < sizeOrigin; ++iCond)
      {
         // Не целесообразно делать условие сильно больше или меньше чем изначальное.
         // Будем брать в пределах двух. Не больше чем в 2 раза и не меньше чем в 2 раза.
         size_t sizeCond = m_rand.Generate(m_original.at(iCond).CountPredicates() / 2, m_original.at(iCond).CountPredicates() * 2);
         SCondition cond;

         for (size_t iPred = 0; iPred < sizeCond; ++iPred)
         {
            SPredicateTemplate predTempl;
            predTempl.idxPredicate = m_rand.Generate(0, idxLastPredicate);
            predTempl.arguments.resize(m_storage.CountArguments(predTempl.idxPredicate));

            if (m_rand.Generate(0, 1))
               cond.left.push_back(std::move(predTempl));
            else
               cond.right.push_back(std::move(predTempl));
         }

         cond.ForEachPredicate([this, &cond](SPredicateTemplate& predTempl)
            {
               FillRandomArguments(predTempl.arguments, cond.maxArgument);
            });

         conds[iCond] = cond;
      }

      m_generation.push_back(std::make_pair(conds, FitnessFunction(conds)));
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

      bringArgumentsBackToNormal(newCond);
      child.push_back(newCond);
   }

   return child;
}

void CGeneticAlgorithm::Selection(TGeneration&& individuals_, size_t countSurvivors_)
{
   if (individuals_.size() < countSurvivors_)
      throw CException("Количество выживших не должно быть меньше самих особей", "Ошибка селекции", "CGeneticAlgorithm::Selection");

   SortGenerationDescendingOrder(individuals_);
   individuals_.resize(countSurvivors_);
   m_generation = std::move(individuals_);   
}

void CGeneticAlgorithm::MutationArguments(TIntegrityLimitation& individual_, double ratio_) const
{
   if (ratio_ <= 0.)
      return;

   size_t countAllArg = 0;
   for (const auto& cond : individual_)
      countAllArg += countAllArgumentsInCondition(cond);

   const size_t iLastCondition = individual_.size() - 1;
   const size_t countVariables = m_storage.CountVariables();

   const size_t countMutations = qMax(static_cast<size_t>(ratio_ * countAllArg), static_cast<size_t>(1));
   for (size_t i = 0; i < countMutations; ++i)
   {
      auto& fullCond = individual_.at(m_rand.Generate(0, iLastCondition)); // выбор условия
      auto& partCond = m_rand.Generate(0, 1) ? fullCond.right : fullCond.left; // выбор части условия (правая или левая)
      if (partCond.empty())
         continue;

      auto& predTempl = partCond.at(m_rand.Generate(0, partCond.size() - 1)); // выбор конкретного предиката в условии целостности
      size_t iArg = m_rand.Generate(0, predTempl.arguments.size() - 1); // выбор позиции (индекса) аргумента у выбранного предиката
      int newValueArg = static_cast<int>(m_rand.Generate(0, fullCond.maxArgument + 2) - 1); // новое значение выбранного аргумента
      if (newValueArg == fullCond.maxArgument + 1)
         ++fullCond.maxArgument;

      predTempl.arguments[iArg] = newValueArg;

      bringArgumentsBackToNormal(fullCond);
   }
}

void CGeneticAlgorithm::MutationPredicates(TIntegrityLimitation& individual_, double ratio_) const
{
   if (ratio_ <= 0.)
      return;

   size_t countPredicats = CountAllPredicates(individual_);

   const size_t iLastCondition = individual_.size() - 1;
   const size_t countPredicates = m_storage.CountPredicates();

   const size_t countMutations = qMax(static_cast<size_t>(ratio_ * countPredicats), static_cast<size_t>(1));
   for (size_t i = 0; i < countMutations; ++i)
   {
      auto& fullCond = individual_.at(m_rand.Generate(0, iLastCondition)); // выбор условия
      auto& partCond = m_rand.Generate(0, 1) ? fullCond.right : fullCond.left; // выбор части условия (правая или левая)
      if (partCond.empty())
         continue;

      size_t idxPredTempl = m_rand.Generate(0, partCond.size() + 1); // выбор конкретного предиката в условии целостности (его индекс в векторе)
      size_t newIdxPredicate = m_rand.Generate(0, countPredicates - 1); // новый индекс предиката
      if (idxPredTempl < partCond.size())
      { // Замена предиката на новый.
         auto& predTempl = partCond.at(idxPredTempl);
         predTempl.idxPredicate = newIdxPredicate; // новый индекс для этого предиката = новый предикат на том же месте

         // Устанавливаем шаблонные аргументы.
         const size_t countArg = m_storage.CountArguments(predTempl.idxPredicate);
         predTempl.arguments.resize(countArg);
         FillRandomArguments(predTempl.arguments, fullCond.maxArgument);
         bringArgumentsBackToNormal(fullCond);
      }
      else if (idxPredTempl == partCond.size())
      { // Добавление предиката.
         const size_t countArg = m_storage.CountArguments(newIdxPredicate);
         SPredicateTemplate newPredTempl(newIdxPredicate, std::vector<int>(countArg));
         FillRandomArguments(newPredTempl.arguments, fullCond.maxArgument);
         partCond.push_back(std::move(newPredTempl));
      }
      else
      { // Удаление предиката.
         partCond.erase(partCond.begin() + m_rand.Generate(0, partCond.size() - 1));
         bringArgumentsBackToNormal(fullCond);
      }
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
   bool bHasEmpty = Cond_.left.empty() || Cond_.right.empty();
   if (bHasEmpty)
      return false;

   bool bNoAnyPred = true; // Нет предикат со всеми аргуентами -1.
   Cond_.ForEachPredicate([&bNoAnyPred](const SPredicateTemplate& predTempl)
      {
         if (bNoAnyPred)
         {
            bool bHasNoAny = false;
            for (int arg : predTempl.arguments)
               if (arg != -1)
               {
                  bHasNoAny = true;
                  break;
               }

            if (!bHasNoAny)
               bNoAnyPred = false;
         }
      });

   return bNoAnyPred;
}

std::pair<bool, bool> CGeneticAlgorithm::IsTrueCondition(const SCondition& Cond_) const
{
   std::vector<SPredicate> vPredLeft, vPredRight;
   for (auto predTemp : Cond_.left)
      vPredLeft.push_back(m_storage.GetPredicate(predTemp.idxPredicate));

   for (auto predTemp : Cond_.right)
      vPredRight.push_back(m_storage.GetPredicate(predTemp.idxPredicate));

   const size_t countVariables = m_storage.CountVariables();

   try
   {
      // Заполняем мапину для предикат имеющих -1 в аргументе.
      std::map<SPredicateTemplate, std::vector<bool>> mapPredAnyArg;
      if (getPredicatesWithAnyArgument(Cond_, mapPredAnyArg))
         return { false, false };


      CCounterWithoutRepeat<size_t> argCounter(0, countVariables, Cond_.maxArgument + 1);
      const size_t countIteration = argCounter.countIterations();
      bool bLeftPartWasTrue = false;
      bool bRightPartWasFalse = false;
      // Перебераем все возможные аргументы.
      for (size_t iteration = 0; iteration < countIteration; ++iteration, ++argCounter)
      {
         // Если в левой части 0, то импликация всегда истинна. (0->X = 1)
         // Если в правой части 1, то импликация тоже всегда истинна. (X->1 = 1)

         const std::vector<size_t>& vSubstitution = argCounter.get();

         // Проверяем для одной подстановки левую часть.
         bool bLeft = true;
         for (size_t i = 0; i < vPredLeft.size(); ++i)
         {
            const auto& predTempl = Cond_.left[i];
            // Ищем в мапине содержащий предикаты с одним или более не зафиксированным аргументом.
            auto it = mapPredAnyArg.find(predTempl);
            if (it != mapPredAnyArg.end())
            {
               std::vector<size_t> fixedArg;
               for (int arg : predTempl.arguments)
                  if (arg != -1)
                     fixedArg.push_back(vSubstitution[arg]);

               size_t idxFixedArg = GetIndex(countVariables, fixedArg);

               if (!it->second.at(idxFixedArg))
               {
                  // Импликация истина.
                  bLeft = false;
                  break;
               }
            }
            else
            {
               // Формируем вектор аргументов из подстановочного вектора.
               std::vector<size_t> arg(predTempl.arguments.size());
               for (size_t j = 0; j < arg.size(); ++j)
                  arg[j] = vSubstitution[predTempl.arguments[j]];

               // Получаем индекс из таблицы.
               size_t idxArg = vPredLeft[i].GetIndex(countVariables, arg);

               // Проверяем
               if (!vPredLeft[i].table.at(idxArg))
               {
                  // Импликация истина.
                  bLeft = false;
                  break;
               }
            }
         }

         if (!bLeft)
         {
            if (bRightPartWasFalse)
               continue; // Уже встретили выражение, где справа был 0.
         }
         else
         {
            // Если мы сюда попали, значит в левой части 1.
            bLeftPartWasTrue = true;
         }

         // В левой части один.
         // Проверяем для одной подстановки правую часть.
         bool bRight = false;
         for (size_t i = 0; i < vPredRight.size(); ++i)
         {
            const auto& predTempl = Cond_.right[i];
            // Ищем в мапине содержащий предикаты с одним или более не зафиксированным аргументом.
            auto it = mapPredAnyArg.find(predTempl);
            if (it != mapPredAnyArg.end())
            {
               std::vector<size_t> fixedArg;
               for (int arg : predTempl.arguments)
                  if (arg != -1)
                     fixedArg.push_back(vSubstitution[arg]);

               size_t idxFixedArg = GetIndex(countVariables, fixedArg);

               if (it->second.at(idxFixedArg))
               {
                  // Импликация истина.
                  bRight = true;
                  break;
               }
            }
            else
            {
               // Формируем вектор аргументов из подстановочного вектора.
               std::vector<size_t> arg(predTempl.arguments.size());
               for (size_t j = 0; j < arg.size(); ++j)
                  arg[j] = vSubstitution[predTempl.arguments[j]];

               // Получаем индекс из таблицы.
               size_t idxArg = vPredRight[i].GetIndex(countVariables, arg);

               // Проверяем
               if (vPredRight[i].table.at(idxArg))
               {
                  // Импликация истина.
                  bRight = true;
                  break;
               }
            }
         }

         if (!bRight)
            bRightPartWasFalse = true;

         if (bLeft && !bRight)
            return { false, true };
      }

      // Цикл по всем набором возможных переменных закончен, значит условие истинно.
      return { true, bLeftPartWasTrue && bRightPartWasFalse };
   }
   catch (std::exception error)
   {
      throw CException(error.what(), "Ошибка проверки условия", "CGeneticAlgorithm::IsTrueCondition");
   }

   return { false, false };
}

bool CGeneticAlgorithm::getPredicatesWithAnyArgument(const SCondition& Cond_, std::map<SPredicateTemplate, std::vector<bool>>& mapPredicates_) const
{
   const size_t countVariables = m_storage.CountVariables();

   bool bHasAnyPred = false; // есть предикат у которого все аргументы -1.

   Cond_.ForEachPredicate([&mapPredicates_, &bHasAnyPred, countVariables, this](const SPredicateTemplate& predTempl)
      {
         if (bHasAnyPred)
            return;

         size_t countAnyArg = 0; // Кол-во аргументов равных -1
         for (int arg : predTempl.arguments)
            if (arg == -1)
               ++countAnyArg;

         if (countAnyArg == 0)
            return;

         if (countAnyArg == predTempl.arguments.size())
         {
            bHasAnyPred = true;
            return;
         }

         // Добавляем предикат в мапину и сохраняем итератор на него.
         std::map<SPredicateTemplate, std::vector<bool>>::iterator it;
         it = mapPredicates_.emplace(predTempl, std::vector<bool>(pow(countVariables, predTempl.arguments.size() - countAnyArg), false)).first;

         // Создаем новый вектор аргументов, который будет нормализирован,
         // т.е. будет содержать шаблоны аргументов по порядку, без пропусков.
         std::vector<int> vTemplArgNormalize(predTempl.arguments.size());
         {
            std::map<int, int> mapReplace;
            mapReplace.emplace(-1, -1);
            int maxArg = -1;
            for (size_t iArg = 0; iArg < vTemplArgNormalize.size(); ++iArg)
            {
               auto itReplace = mapReplace.find(predTempl.arguments[iArg]);
               if (itReplace == mapReplace.end())
                  itReplace = mapReplace.emplace(predTempl.arguments[iArg], ++maxArg).first;

               vTemplArgNormalize[iArg] = itReplace->second;
            }
         }

         SPredicate predicate = m_storage.GetPredicate(predTempl.idxPredicate);
         CCounterWithoutRepeat<size_t> counterArg(0, countVariables, predTempl.arguments.size() - countAnyArg); // счетчик для перебора аргументов не равных -1
         const size_t countIteration = counterArg.countIterations();
         const size_t countAnyIter = CCounterWithoutRepeat<size_t>(0, countVariables, countAnyArg).countIterations();
         for (size_t iteration = 0; iteration < countIteration; ++iteration, ++counterArg)
         {
            const std::vector<size_t>& vSubstitution = counterArg.get(); // вектор подстановки (из шаблона - реальный аргумент)

            std::vector<int> arg(vTemplArgNormalize.size(), -1); // вектор с подставленными аргументами (реальными) и -1.
            std::vector<size_t> fixedArg; // вектор хранящий только зафиксированные аргументы (те которые не равны -1).
            for (size_t iArg = 0; iArg < vTemplArgNormalize.size(); ++iArg)
            {
               if (vTemplArgNormalize[iArg] != -1)
               {
                  const size_t realArg = vSubstitution[static_cast<size_t>(vTemplArgNormalize[iArg])];
                  arg[iArg] = static_cast<int>(realArg);
                  fixedArg.push_back(realArg);
               }
            }

            CCounterWithoutRepeat<size_t> counterAnyArg(0, countVariables, countAnyArg); // счетчик для перебора аргуметов на место -1
            for (size_t iterAny = 0; iterAny < countAnyIter; ++iterAny, ++counterAnyArg)
            {
               // Подставляем вместо -1 аргументы полученные для текущей итерации.
               std::vector<size_t> vSubstAny = counterAnyArg.get();
               size_t currentIdxSubst = 0;
               std::vector<size_t> argInstance(arg.size()); // вектор полностью подставленных аргументов (реальных)
               for (size_t iArg = 0; iArg < arg.size(); ++iArg)
               {
                  if (arg[iArg] == -1)
                     argInstance[iArg] = vSubstAny[currentIdxSubst++];
                  else
                     argInstance[iArg] = arg[iArg];
               }

               size_t indexArg = predicate.GetIndex(countVariables, argInstance);
               if (predicate.table[indexArg])
               {
                  // Нашли хотя бы один экземпляр аргументов, для которых значение истинно.
                  it->second[GetIndex(countVariables, fixedArg)] = true;
                  break;
               }
            }
         }
      });

   return bHasAnyPred;
}

size_t CGeneticAlgorithm::SelectRandParent() const
{
   if (m_generation.size() < 2)
      throw CException("Слишком мало индивидуумов!", "Ошибка выбора родителя", "CGeneticAlgorithm::SelectRandParent");

   m_rand.SetBoundaries(0, m_generation.size() - 1);

   qint64 first = m_rand.Generate();
   qint64 second = m_rand.Generate();
   while (first == second)
      second = m_rand.Generate();

   return m_generation[first].second < m_generation[second].second ? second : first;
}

std::pair<size_t, size_t> CGeneticAlgorithm::GetPairParents() const
{
   size_t index1 = SelectRandParent();
   size_t index2 = SelectRandParent();

   while (index1 == index2)
      index2 = SelectRandParent();

   return std::make_pair(index1, index2);
}

void CGeneticAlgorithm::SortGenerationDescendingOrder(TGeneration& generation_) const
{
   std::sort(generation_.begin(), generation_.end(),
      [](const TIntLimAndFitness& a, const TIntLimAndFitness& b)
      {
         return a.second > b.second;
      });
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
      std::pair<bool, bool> correct = IsTrueCondition(conds_.at(iCond));
      double fitnesCond = correct.first ? correct.second ? 0. : -0.75 : -1.;
      fitnesCond += dMultiplierArgs * count.matchPred;
      fitnesCond += m_costAddingPredicate * count.addedPred;
      fitnesCond /= count.matchPred + count.addedPred + count.deletedPred;

      fitnes += fitnesCond / m_original.size();
   }

   return fitnes;
}

std::multimap<int, size_t> CGeneticAlgorithm::findMinDifference(const SPredicateTemplate& sample_, const TPartCondition& verifiable_) const
{
   // Число отличий, индекс в векторе условия.
   std::multimap<int, size_t> result;

   try
   {
      const size_t countVariables = m_storage.CountVariables();
      const std::vector<int>& sampleArgs = sample_.arguments;

      if (sampleArgs.empty())
         throw CException("Предикат с 0 аргументов недопустим.", "Ошибка", "CGeneticAlgorithm::findMinDifference");

      for (size_t iPredTempl = 0; iPredTempl < verifiable_.size(); ++iPredTempl)
      {
         const SPredicateTemplate& predTempl = verifiable_.at(iPredTempl);
         if (predTempl.idxPredicate != sample_.idxPredicate)
            continue;

         const std::vector<int>& verArgs = predTempl.arguments;

         if (verArgs.empty() || verArgs.size() != sampleArgs.size())
            throw CException("", "Ошибка. Обратитесь к разработчику", "CGeneticAlgorithm::findMinDifference");

         int counter = 0;
         for (size_t i = 0; i < verArgs.size(); ++i)
         {
            if (sampleArgs[i] != verArgs[i])
               ++counter;
         }

         result.emplace(counter, iPredTempl);
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
            const size_t idxPredTempl = candidate.second;
            if (!vUsedPredInst.at(idxPredTempl))
            {
               vUsedPredInst.at(idxPredTempl) = true;
               count.diffArg += candidate.first;
               count.totalArg += verifiable_.at(idxPredTempl).arguments.size();
               bFoundPair = true;
               break;
            }
         }

         if (bFoundPair)
            ++count.matchPred;
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
   if (total_ == 0)
      return 0;

   return 1. - (differences_ * (1. - m_minCostForArgDif) / total_);
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

CGeneticAlgorithm::SCounts& CGeneticAlgorithm::SCounts::operator+=(const SCounts& added_)
{
   diffArg += added_.diffArg;
   totalArg += added_.totalArg;

   matchPred += added_.matchPred;
   addedPred += added_.addedPred;
   deletedPred += added_.deletedPred;

   return *this;
}