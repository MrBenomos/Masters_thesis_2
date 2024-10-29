#include "genetic_algorithm.h"
#include <QFile>
#include <QTextStream>

CGeneticAlgorithm::CGeneticAlgorithm()
{
   m_rand.UseNewNumbers();
}

bool CGeneticAlgorithm::AddVariable(const SPropVar& Variable_)
{
   if (!m_vVariables.empty())
      throw QString("Невозможно добавить переменную, после добавления условия(ий).");

   return m_mapVariables.emplace(Variable_, static_cast<size_t>(0)).second;
}

size_t CGeneticAlgorithm::AddCondition()
{
   if (m_vVariables.size() != m_mapVariables.size())
      if (m_vVariables.empty())
         InitVectVar();
      else
         throw QString("Обнаружена новая переменная, при уже сформированном векторе переменных.");

   m_vSpecified.push_back(TCond(m_vVariables.size(), 0));
   return m_vSpecified.size() - 1;
}

EAddingError CGeneticAlgorithm::AddVariableLeftSideCondition(size_t IndexCondition_, const QString& Name_)
{
   auto itVar = m_mapVariables.find(SPropVar(Name_, false));
   if (itVar == m_mapVariables.end())
      return eUnknownVariable;

   if (IndexCondition_ >= m_vSpecified.size())
      return eInvalidIndex;

   TCond& cond = m_vSpecified[IndexCondition_];

   if (itVar->second >= cond.size())
      return eInvalidIndex;

   char& var = cond[itVar->second];

   if (var == 1)
      return VariablePresent;

   if (var == 2)
      return OppositeCondition;

   var = 1;

   return eSuccessfully;
}

EAddingError CGeneticAlgorithm::AddVariableRightSideCondition(size_t IndexCondition_, const QString& Name_)
{
   auto itVar = m_mapVariables.find(SPropVar(Name_, false));
   if (itVar == m_mapVariables.end())
      return eUnknownVariable;

   if (IndexCondition_ >= m_vSpecified.size())
      return eInvalidIndex;

   TCond& cond = m_vSpecified[IndexCondition_];

   if (itVar->second >= cond.size())
      return eInvalidIndex;

   char& var = cond[itVar->second];

   if (var == 2)
      return VariablePresent;

   if (var == 1)
      return OppositeCondition;

   var = 2;

   return eSuccessfully;
}

bool CGeneticAlgorithm::SetVariablesFromString(const QString& Str_, QString& StrError_)
{
   std::vector<SPropVar> vVar;
   size_t length = Str_.length();
   for (size_t i = 0; i < length;)
   {
      // Отбрасываем все пробелы
      while (i < length && Str_.at(i).isSpace())
         ++i;

      if (i >= length || Str_.at(i) == '#')
         break;

      // Считываем имя переменной
      if (!Str_.at(i).isLetter())
      {
         StrError_ = ErrorMessage("Имя переменной может начинаться только с латинской буквы.", Str_, i);
         return false;
      }

      QString name;
      for (; i < length; ++i)
      {
         QChar symb = Str_.at(i);
         if (IllegalSymbol(symb) || symb.isSpace())
            break;

         name += symb;
      }

      // Отбрасываем все пробелы
      while (i < length && Str_.at(i).isSpace())
         ++i;

      if (i >= length || Str_.at(i) == '#')
      {
         StrError_ = ErrorMessage("Незаконченная строка, ожидалось \'=\'.", Str_);
         return false;
      }

      // Получаем символ '='
      if (Str_.at(i) != '=')
      {
         StrError_ = ErrorMessage("Ожидалось \'=\'.", Str_, i);
         return false;
      }

      ++i;

      // Отбрасываем все пробелы
      while (i < length && Str_.at(i).isSpace())
         ++i;

      if (i >= length || Str_.at(i) == '#')
      {
         StrError_ = ErrorMessage("Незаконченная строка, ожидалось значение переменной.", Str_);
         return false;
      }

      // Считываем значение
      QChar symb = Str_.at(i);
      if (!symb.isDigit())
      {
         StrError_ = ErrorMessage("Ожидалось \'0\' или \'1\'.", Str_, i);
         return false;
      }

      // Добавляем переменную
      vVar.push_back(SPropVar(name, symb == '0' ? false : true));

      ++i;

      // Отбрасываем все пробелы до ','
      while (i < length && Str_.at(i).isSpace())
         ++i;

      // Отбрасываем ','
      if (i < length && Str_.at(i) == ',')
         ++i;
   }

   QString existing;
   size_t count = 0;
   for (const auto& var : vVar)
   {
      if (!AddVariable(var))
      {
         if (count > 0)
            existing += ", \'" + var.name + '\'';
         else
            existing = '\'' + var.name + '\'';

         ++count;
      }
   }

   if (count > 0)
   {
      if (count == 1)
         StrError_ = ErrorMessage("Переменная " + existing + " уже существует", Str_);
      else
         StrError_ = ErrorMessage("Переменные " + existing + " уже существуют", Str_);
   }

   return true;
}

bool CGeneticAlgorithm::SetConditionFromString(const QString& Str_, QString& StrError_)
{
   bool isLeftPart = true;
   bool isFirstVar = true;
   size_t idx = SIZE_MAX;
   size_t length = Str_.length();
   for (size_t i = 0; i < length;)
   {
      // Отбрасываем все пробелы
      while (i < length && (Str_.at(i).isSpace() || Str_.at(i) == '$'))
         ++i;

      if (i >= length || Str_.at(i) == '#')
         if ((isLeftPart && !isFirstVar) || (!isLeftPart && isFirstVar))
         {
            StrError_ = ErrorMessage("Отсутствует правая часть условия.", Str_);
            if (idx != SIZE_MAX)
               m_vSpecified.erase(m_vSpecified.begin() + idx);
            return false;
         }
         else
            break;

      // Считываем имя
      if (!Str_.at(i).isLetter())
      {
         StrError_ = ErrorMessage("Имя переменной может начинаться только с латинской буквы.", Str_, i);
         if (idx != SIZE_MAX)
            m_vSpecified.erase(m_vSpecified.begin() + idx);
         return false;
      }

      QString name;
      for (; i < length; ++i)
      {
         QChar symb = Str_.at(i);
         if (IllegalSymbol(symb) || symb.isSpace())
            break;

         name += symb;
      }

      // Добавляем переменную к условию
      EAddingError errorIdx;
      if (isLeftPart)
      {
         if (isFirstVar)
         {
            idx = AddCondition();
            isFirstVar = false;
         }

         errorIdx = AddVariableLeftSideCondition(idx, name);
      }
      else
      {
         if (isFirstVar)
            isFirstVar = false;

         errorIdx = AddVariableRightSideCondition(idx, name);
      }

      switch (errorIdx)
      {
      case eSuccessfully:
         break;
      case eUnknownVariable:
         StrError_ = ErrorMessage("Неинициализированная переменная \'" + name + "\'.", Str_);
         if (idx != SIZE_MAX)
            m_vSpecified.erase(m_vSpecified.begin() + idx);
         return false;
      case VariablePresent:
         break;
      case OppositeCondition:
         StrError_ = ErrorMessage("Переменная \'" + name + "\' уже использовалась в другой части этого условия.", Str_);
         if (idx != SIZE_MAX)
            m_vSpecified.erase(m_vSpecified.begin() + idx);
         return false;
      case eInvalidIndex:
         StrError_ = ErrorMessage("Обратитесь к разработчику (ошибка индексации, при создании условия).", Str_);
         if (idx != SIZE_MAX)
            m_vSpecified.erase(m_vSpecified.begin() + idx);
         return false;
      default:
         StrError_ = ErrorMessage("Неизвестная ошибка.", Str_);
         if (idx != SIZE_MAX)
            m_vSpecified.erase(m_vSpecified.begin() + idx);
         return false;
      }


      // Отбрасываем все пробелы
      while (i < length && Str_.at(i).isSpace())
         ++i;

      if (i >= length || Str_.at(i) == '#')
         if (isLeftPart)
         {
            StrError_ = ErrorMessage("Отсутствует правая часть условия.", Str_);
            if (idx != SIZE_MAX)
               m_vSpecified.erase(m_vSpecified.begin() + idx);
            return false;
         }
         else
            break;

      if (Str_.at(i) == ',')
      {
         ++i;
         continue;
      }

      if (Str_.at(i) == '-')
      {
         if (!isLeftPart)
         {
            StrError_ = ErrorMessage("В строке может быть только одно условие", Str_, i);
            if (idx != SIZE_MAX)
               m_vSpecified.erase(m_vSpecified.begin() + idx);
            return false;
         }

         ++i;
         if (i < length)
            if (Str_.at(i) == '>')
            {
               ++i;
               isLeftPart = false;
               isFirstVar = true;
               continue;
            }
            else
            {
               StrError_ = ErrorMessage("Ожидалось \'>\'.", Str_, i);
               if (idx != SIZE_MAX)
                  m_vSpecified.erase(m_vSpecified.begin() + idx);
               return false;
            }
         else
         {
            StrError_ = ErrorMessage("Отсутствует правая часть условия.", Str_);
            if (idx != SIZE_MAX)
               m_vSpecified.erase(m_vSpecified.begin() + idx);
            return false;
         }
      }

   }

   if ((isLeftPart && !isFirstVar) || (!isLeftPart && isFirstVar))
   {
      StrError_ = ErrorMessage("Отсутствует правая часть условия.", Str_);
      if (idx != SIZE_MAX)
         m_vSpecified.erase(m_vSpecified.begin() + idx);
      return false;
   }

   return true;
}

bool CGeneticAlgorithm::FillDataInFile(const QString& FileName_, QString& StrError_)
{
   QFile file(FileName_);
   if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
   {
      StrError_ = "Не удалось открыть файл: " + FileName_;
      return false;
   }

   Clear();

   QTextStream in(&file);
   bool isVariables = true;
   while (!in.atEnd()) // Цикл чтения строк из потока
   {
      QString line = in.readLine();
      if (line.isNull() || line.isEmpty())
         continue;
      for (auto i = 0; i < line.size(); ++i)
         if (!line.at(i).isSpace())
         {
            if (line.at(i) == '$')
               isVariables = false;

            break;
         }

      if (isVariables)
      {
         if (!SetVariablesFromString(line, StrError_))
         {
            Clear();
            return false;
         }
      }
      else
      {
         if (!SetConditionFromString(line, StrError_))
         {
            Clear();
            return false;
         }
      }

   }

   if (isVariables)
   {
      StrError_ = ErrorMessage("Отсутствует ограничение целостности");
      Clear();
      return false;
   }

   return true;
}

bool CGeneticAlgorithm::WriteСonditionIntegrityInFile(const QString& FileName_, QString& StrError_) const
{
   QFile file(FileName_);

   if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
   {
      StrError_ = "Не удалось открыть файл: " + FileName_;
      return false;
   }

   QTextStream out(&file);

   if (!WriteVarsInStream(out, StrError_))
      return false;

   out << Qt::endl << '$' << Qt::endl;

   return WriteСondsInStream(out, StrError_, m_vSpecified);
}

bool CGeneticAlgorithm::WriteGenerationsInFile(const QString& FileName_, QString& StrError_) const
{
   QFile file(FileName_);

   if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
   {
      StrError_ = "Не удалось открыть файл: " + FileName_;
      return false;
   }

   QTextStream out(&file);

   // Переменные
   if (!WriteVarsInStream(out, StrError_))
      return false;

   out << Qt::endl << '$';

   const size_t sizeGen = m_vGenerations.size();
   for (size_t i = 0; i < sizeGen; ++i)
   {
      out << Qt::endl << "# " << i << "\t = " << FitnessFunction(m_vGenerations[i]) << Qt::endl;
      if (!WriteСondsInStream(out, StrError_, m_vGenerations[i]))
      {
         StrError_ += "\nОбъект " + QString::number(i) + " из " + QString::number(sizeGen);
         return false;
      }
   }

   return true;
}

bool CGeneticAlgorithm::GetVariables(QString& strVariables_, QString& strError_) const
{
   QTextStream out(&strVariables_);
   return WriteVarsInStream(out, strError_);
}

bool CGeneticAlgorithm::GetСondIntegrity(QString& strСondIntegrity_, QString& strError_) const
{
   QTextStream out(&strСondIntegrity_);
   return WriteСondsInStream(out, strError_, m_vSpecified);
}

bool CGeneticAlgorithm::GetAllGenerations(QString& strGenerations_, QString& strError_) const
{
   QTextStream out(&strGenerations_);

   const size_t sizeGen = m_vGenerations.size();
   for (size_t i = 0; i < sizeGen; ++i)
   {
      out << "# " << i << "\t = " << FitnessFunction(m_vGenerations[i]) << Qt::endl;
      if (!WriteСondsInStream(out, strError_, m_vGenerations[i]))
      {
         strError_ += "\nОбъект " + QString::number(i) + " из " + QString::number(sizeGen);
         return false;
      }
      out << Qt::endl;
   }

   // Удаление последнего перехода на новую строку.
   if (strGenerations_.endsWith("\r\n"))
      strGenerations_.chop(2);
   else if (strGenerations_.endsWith("\n"))
      strGenerations_.chop(1);

   return true;
}

bool CGeneticAlgorithm::GetVarAndCond(QString& str_, QString& strError_) const
{
   if (!GetVariables(str_, strError_))
      return false;

   QTextStream out(&str_);
   out << Qt::endl << "$" << Qt::endl;

   return GetСondIntegrity(str_, strError_);
}

bool CGeneticAlgorithm::GetVarAndGen(QString& str_, QString& strError_) const
{
   if (!GetVariables(str_, strError_))
      return false;

   QTextStream out(&str_);
   out << Qt::endl << "$" << Qt::endl;

   return GetAllGenerations(str_, strError_);
}

void CGeneticAlgorithm::Start(unsigned int CountIndividuals_, size_t CountIterations_, bool UseMutation_, unsigned int Percent_)
{
   if (UseMutation_ && Percent_ == 0)
      UseMutation_ = false;

   const double percentagePerIteration = 100. / CountIterations_; // количество процентов за одну итерацию
   int percentageCompleted = 0;

   // Создание первого поколения
   CreateFirstGenerationRandom(CountIndividuals_);

   for (size_t iGeneration = 0; iGeneration < CountIterations_; ++iGeneration)
   {
      std::vector<TСondIntegrity> children;
      for (size_t iNewIndiv = 0; iNewIndiv < CountIndividuals_ * 2; ++iNewIndiv)
      {
         m_rand.SetBoundaries(0, CountIndividuals_ - 1);

         // Селекция (выбор родителей) (турнирный отбор)
         size_t idxParent1, idxParent2;

         size_t first = m_rand.Generate();
         size_t second = m_rand.Generate();
         while (first == second)
            second = m_rand.Generate();

         idxParent1 = FitnessFunction(m_vGenerations[first]) > FitnessFunction(m_vGenerations[second]) ? first : second;

         first = m_rand.Generate();
         second = m_rand.Generate();
         while (first == second)
            second = m_rand.Generate();

         idxParent2 = FitnessFunction(m_vGenerations[first]) > FitnessFunction(m_vGenerations[second]) ? first : second;

         // Скрещивание
         children.push_back(CrossingByGenes(m_vGenerations[idxParent1], m_vGenerations[idxParent2]));
      }

      // Мутация (отключена для последних итераций)
      if (UseMutation_ && iGeneration < CountIterations_ - 1)
         for (auto& individ : children)
            Mutation(individ, 0.01 * Percent_);

      // Селекция (полная замена, родителей "убиваем")
      Selection(children, CountIndividuals_);

      // Отправляем сигнал о проценте выполнения.
      // +1 не делаю умышленно, чтобы последняя итерация не была = 100.
      if (percentagePerIteration * iGeneration > percentageCompleted)
         Q_EMIT signalProgressUpdate(++percentageCompleted);
   }

   // Сортируем в порядке убывания
   SortDescendingOrder();

   Q_EMIT signalProgressUpdate(100);
}

void CGeneticAlgorithm::StartForThread(unsigned int CountIndividuals_, size_t CountIterations_, bool UseMutation, unsigned int Percent_)
{
   try
   {
      Start(CountIndividuals_, CountIterations_, UseMutation, Percent_);
   }
   catch (const QString strError)
   {
      Q_EMIT signalError(strError);
   }

   Q_EMIT signalEnd();
}

void CGeneticAlgorithm::Clear()
{
   m_mapVariables.clear();
   m_vVariables.clear();
   m_vSpecified.clear();
   m_vGenerations.clear();
}

bool CGeneticAlgorithm::HasGenerations() const
{
   return !m_vGenerations.empty();
}

void CGeneticAlgorithm::InitVectVar()
{
   m_vVariables.clear();

   size_t pos = 0;
   for (auto& var : m_mapVariables)
   {
      m_vVariables.push_back(var.first);
      var.second = pos++;
   }
}

bool CGeneticAlgorithm::IllegalSymbol(QChar Symbol_)
{
   const QChar illegalSymbols[] = { '=', ',', '-','>', '$', '#' };

   for (const auto& illegal : illegalSymbols)
      if (Symbol_ == illegal)
         return true;

   return false;
}

QString CGeneticAlgorithm::ErrorMessage(const QString& Message_, const QString& Line_, size_t Position_)
{
   QString strError("Ошибка.");

   if (!Message_.isEmpty())
   {
      strError += ' ';
      strError += Message_;
   }

   if (Position_ != SIZE_MAX && Position_ < Line_.size())
   {
      strError += "\nВстречен символ \'";
      strError += Line_.at(Position_);
      strError += "\' в позиции: ";
      strError += QString::number(Position_ + 1) + '.';
   }

   if (!Line_.isEmpty())
   {
      strError += "\nСтрока:";
      strError += Line_;
   }

   return strError;
}

bool CGeneticAlgorithm::WriteVarsInStream(QTextStream& Stream_, QString& StrError_) const
{
   if (!m_vVariables.empty())
   {
      Stream_ << m_vVariables[0].name << " = " << (m_vVariables[0].value ? '1' : '0');

      for (int i = 1; i < m_mapVariables.size(); ++i)
         Stream_ << ", " << m_vVariables[i].name << " = " << ((m_vVariables.at(i).value) ? '1' : '0');
   }
   else
   {
      StrError_ = ErrorMessage("Нет переменных.");
      return false;
   }

   return true;
}

bool CGeneticAlgorithm::WriteСondsInStream(QTextStream& Stream_, QString& StrError_, const TСondIntegrity& Conds_) const
{
   if (!m_vVariables.empty())
   {
      if (!Conds_.empty())
      {
         bool isFirst = true;

         for (const auto& cond : Conds_)
         {
            if (isFirst)
               isFirst = false;
            else
               Stream_ << Qt::endl;

            bool isFirst = true;
            for (int i = 0; i < cond.size(); ++i)
            {
               if (cond[i] == 1)
                  if (isFirst)
                  {
                     Stream_ << m_vVariables[i].name;
                     isFirst = false;
                  }
                  else
                     Stream_ << ", " << m_vVariables[i].name;
            }

            Stream_ << " -> ";

            isFirst = true;
            for (int i = 0; i < cond.size(); ++i)
            {
               if (cond[i] == 2)
                  if (isFirst)
                  {
                     Stream_ << m_vVariables[i].name;
                     isFirst = false;
                  }
                  else
                     Stream_ << ", " << m_vVariables[i].name;
            }
         }
      }
      else
      {
         StrError_ = ErrorMessage("Нет условий.");
         return false;
      }
   }
   else
   {
      StrError_ = ErrorMessage("Нет переменных.");
      return false;
   }

   return true;
}

void CGeneticAlgorithm::CreateFirstGenerationRandom(size_t Count_)
{
   if (Count_ < 2)
      throw ErrorMessage("Количество особей должно быть больше 1");

   if (m_vVariables.size() < 2)
      throw ErrorMessage("Количество переменных должно быть больше 1");

   const size_t sizeCond = m_vSpecified.size();
   const size_t sizeVar = m_vVariables.size();

   m_vGenerations.clear();

   for (size_t iGen = 0; iGen < Count_; ++iGen)
   {
      TСondIntegrity conds(sizeCond);

      for (size_t iCond = 0; iCond < sizeCond; ++iCond)
      {
         TCond cond(sizeVar);

         do
         {
            for (size_t iVar = 0; iVar < sizeVar; ++iVar)
            {
               cond[iVar] = static_cast<char>(m_rand.Generate(0, 2));
            }
         } while (!IsCorrectCondition(cond));

         conds[iCond] = cond;
      }

      m_vGenerations.push_back(conds);
   }
}

CGeneticAlgorithm::TСondIntegrity CGeneticAlgorithm::CrossingByGenes(const TСondIntegrity& Parent1_, const TСondIntegrity& Parent2_) const
{
   if (Parent1_.size() != Parent2_.size())
      throw ErrorMessage("Разное количество условий целостности у родителей!");

   TСondIntegrity child;

   for (size_t iCond = 0; iCond < Parent1_.size(); ++iCond)
   {
      TCond cond(m_vVariables.size());

      for (size_t iVar = 0; iVar < m_vVariables.size(); ++iVar)
         cond[iVar] = m_rand.Generate(1, 2) == 1 ? Parent1_[iCond][iVar] : Parent2_[iCond][iVar];

      child.push_back(cond);
   }

   return child;
}

CGeneticAlgorithm::TСondIntegrity CGeneticAlgorithm::CrossingByConds(const TСondIntegrity& Parent1_, const TСondIntegrity& Parent2_) const
{
   if (Parent1_.size() != Parent2_.size())
      throw ErrorMessage("Разное количество условий целостности у родителей!");

   TСondIntegrity child;

   for (size_t iCond = 0; iCond < Parent1_.size(); ++iCond)
      child.push_back(m_rand.Generate(1, 2) == 1 ? Parent1_[iCond] : Parent2_[iCond]);

   return child;
}

void CGeneticAlgorithm::Selection(const std::vector<TСondIntegrity>& Individuals_, size_t CountSurvivors_)
{
   if (Individuals_.size() < CountSurvivors_)
      throw ErrorMessage("Количество выживших не должно быть меньше самих особей");

   std::multimap<double, size_t, std::greater<double>> mapIdx;

   for (size_t iIndiv = 0; iIndiv < Individuals_.size(); ++iIndiv)
      mapIdx.emplace(FitnessFunction(Individuals_[iIndiv]), iIndiv);

   m_vGenerations.clear();

   auto itIdx = mapIdx.begin();

   for (size_t i = 0; i < CountSurvivors_; ++i)
      m_vGenerations.push_back(Individuals_[(itIdx++)->second]);
}

void CGeneticAlgorithm::Mutation(TСondIntegrity& Individual_, double Ratio_) const
{
   if (Ratio_ <= 0.)
      return;

   const size_t countConds = m_vSpecified.size();
   const size_t countVar = m_vVariables.size();
   const size_t countMutations = std::max(static_cast<int>(Ratio_ * countConds * countVar), 1);

   for (int i = 0; i < countMutations; ++i)
   {
      size_t idxCond = m_rand.Generate(0, (int)countConds-1);
      size_t idxVar = m_rand.Generate(0, (int)countVar-1);
      char newVar = m_rand.Generate(0, 2);

      Individual_[idxCond][idxVar] = newVar;
   }
}

bool CGeneticAlgorithm::IsCorrectCondition(const TCond& Cond_) const
{
   bool haveLeft = false;
   bool haveRight = false;
   for (const auto& var : Cond_)
   {
      if (var == 1)
         haveLeft = true;
      else if (var == 2)
         haveRight = true;

      if (haveLeft && haveRight)
         return true;
   }

   return haveLeft && haveRight;
}

bool CGeneticAlgorithm::IsTrueCondition(const TCond& Cond_) const
{
   for (int i = 0; i < Cond_.size(); ++i)
   {
      if (Cond_[i] == 1 && !m_vVariables[i].value)
         return true;
      else if (Cond_[i] == 2 && m_vVariables[i].value)
         return true;
   }

   return false;
}

double CGeneticAlgorithm::FitnessFunction(const TСondIntegrity& Conds_) const
{
   size_t countVar = m_vVariables.size();

   double fitness = 0;
   const double maxCondition = 1. / Conds_.size();

   for (int iCond = 0; iCond < Conds_.size(); ++iCond)
   {
      const TCond& cond = Conds_[iCond];
      if (!IsCorrectCondition(cond))
         continue;

      // Считаем сколько отличий
      size_t n = 0;
      for (int iVar = 0; iVar < cond.size(); ++iVar)
      {
         switch (cond[iVar])
         {
         case 0:
            if (m_vSpecified[iCond][iVar] != 0)
               ++n;
            break;
         case 1:
            if (m_vSpecified[iCond][iVar] == 0)
               ++n;
            else if (m_vSpecified[iCond][iVar] == 2)
               n += 2;
            break;
         case 2:
            if (m_vSpecified[iCond][iVar] == 0)
               ++n;
            else if (m_vSpecified[iCond][iVar] == 1)
               n += 2;
            break;
         default:
            throw;
         }
      }

      if (!IsTrueCondition(cond))
         fitness += maxCondition / (((countVar + 1) * 2) + n);
      else
         fitness += maxCondition / (n + 1);
   }

   return fitness;
}

void CGeneticAlgorithm::SortDescendingOrder()
{
   std::multimap<double, TСondIntegrity, std::greater<double>> mapIdx;

   for (const auto& individ : m_vGenerations)
      mapIdx.emplace(FitnessFunction(individ), individ);

   m_vGenerations.clear();

   for (const auto& individ : mapIdx)
      m_vGenerations.push_back(individ.second);

}
