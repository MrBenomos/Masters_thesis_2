#include <QTextStream>

#include "custom_math.h"
#include "predicate.h"
#include "exception.h"
#include "counter.h"
#include "global.h"

// Неверный индекс предиката. #1 - кол-во предикатов, #2 - индекс к которому пытались обратиться.
static const QString INVALID_PREDICATE("Попытка обращения к предикату с несуществующим индексом. Всего предикатов: %1, попытка обращения к: %2");
// Неверный индекс таблицы истинности. #1 - Имя предиката, #2 - кол-во строк в таблице, #3 - индекс к которому пытались обратиться.
static const QString INVALID_TABLE("Попытка обращения к таблице истинности с несуществующим индексом у предиката \"%1\". Всего строк: %2, попытка обращения к: %3");
// Ошибка: невозможно изменить переменные.
static const CException EXC_CANT_ADD_VARIABLE("Невозможно изменить набор переменных, после добавления хотя бы одного предиката.", "Ошибка изменения переменых", "CPredicatesStorage::SetVariables");

constexpr const char RESERVED_CHARACTERS[] = "(),;";

size_t GetIndex(size_t countVariables_, const std::vector<size_t>& args_)
{
   size_t index = 0;
   for (const auto& arg : args_)
   {
      index *= countVariables_;
      index += arg;
   }

   return index;
}

size_t SPredicate::GetIndex(size_t countVariables_, const std::vector<size_t>& args_) const
{
   if (pow(static_cast<size_t>(countVariables_), args_.size()) != table.size())
      return SIZE_MAX;

   size_t index = 0;
   for (const auto& arg : args_)
   {
      index *= countVariables_;
      index += arg;
   }

   if (index >= table.size())
      return SIZE_MAX;

   return index;
}

const std::vector<size_t> SPredicate::GetArgs(size_t countVariables_, size_t indexArguments_) const
{
   std::vector<size_t> vArgs;

   size_t countArg = intLog(countVariables_, table.size());

   if (pow(countVariables_, static_cast<size_t>(countArg)) != table.size())
      return vArgs;

   vArgs.resize(countArg);
   for (size_t i = 1; i <= countArg; ++i)
   {
      vArgs[countArg - i] = indexArguments_ % countVariables_;
      indexArguments_ /= countVariables_;
   }
   
   return vArgs;
}

void CPredicatesStorage::SetVariables(const std::set<QString>& variables_)
{
   if (!m_mapPredicates.empty() || !m_vPredicates.empty())
      throw EXC_CANT_ADD_VARIABLE;


   m_vVariables.resize(variables_.size());

   size_t idx = 0;
   for (const auto& var : variables_)
   {
      if (var.isEmpty())
         throw CException("Некорректное имя переменной.", "Ошибка изменения переменых", "CPredicatesStorage::SetVariables");

      m_vVariables[idx] = var;
      m_mapVariables.emplace(var, idx++);
   }
}

bool CPredicatesStorage::SetVariables(const std::vector<QString>& variables_)
{
   if (!m_mapPredicates.empty() || !m_vPredicates.empty())
      throw EXC_CANT_ADD_VARIABLE;

   std::set<QString> setName;
   for (const auto& var : variables_)
      setName.emplace(var);

   SetVariables(setName);

   return variables_.size() == m_vVariables.size();
}

void CPredicatesStorage::SetVariables(const QString& str_)
{
   if (!m_mapPredicates.empty() || !m_vPredicates.empty())
      throw EXC_CANT_ADD_VARIABLE;

   std::set<QString> setName;

   for (qsizetype i = 0; i < str_.size(); /*++i*/)
   {
      skipSpace(str_, i);
      QString name = highlightName(str_, i);

      if (name.isEmpty())
      {
         if (i < str_.size())
            throw CException(QString("Ожидалось имя переменной. Встречен символ \'%1\'").arg(str_.at(i)), "Ошибка добавления переменых", "CPredicatesStorage::SetVariables");
         else
            break;
      }

      setName.emplace(name);
      skipSpace(str_, i, ',');
   }

   SetVariables(setName);
}

void CPredicatesStorage::AddPredicates(const QString& str_)
{
   if (m_vVariables.empty() || m_mapVariables.empty())
      throw CException("Невозможно создать предикат, список всех переменных пуст!", "Ошибка добавления предиката", "CPredicatesStorage::AddPredicates");

   qsizetype length = str_.size();
   QString nameNextPredicate;

   // Цикл по предикатам.
   for (qsizetype i = 0; i < length; /*++i*/)
   {
      SPredicate predicate;

      // имя предиката

      if (nameNextPredicate.isEmpty())
      {

         if (skipSpace(str_, i))
            break;

         predicate.name = highlightName(str_, i);
      }
      else
      {
         predicate.name = nameNextPredicate;
         nameNextPredicate.clear();
      }

      if (predicate.name.isEmpty())
         throw CException("Некорректное имя предиката.", "Ошибка добавления предиката", "CPredicatesStorage::AddPredicates");

      // проверка существования
      auto it = m_mapPredicates.find(predicate.name);
      if (it != m_mapPredicates.end())
         throw CException(QString("Попытка добавить предикат с уже существующим именем \"%1\".").arg(predicate.name), "Ошибка добавления предиката", "CPredicatesStorage::AddPredicates");

      skipSpace(str_, i, '(');

      if (skipSpace(str_, i))
         throw CException(QString("Предикат \"%1\" не закончен. Нет количества аргументов и таблицы истинности.").arg(predicate.name), "Ошибка добавления предиката", "CPredicatesStorage::AddPredicates");

      // число (кол-во аргументов)

      qint16 numberArg = 0;

      while (i < length && str_.at(i).isDigit())
      {
         numberArg *= 10;
         numberArg += str_.at(i).digitValue();
         ++i;
      }

      if (numberArg == 0)
         throw CException(QString("Ожидалось количество аргументов у предиката \"%1\". У предиката должо быть не менее 1 аргумента.").arg(predicate.name), "Ошибка добавления предиката", "CPredicatesStorage::AddPredicates");

      skipSpace(str_, i, ')');

      // таблица истинности
      size_t tableSize = pow(m_vVariables.size(), static_cast<size_t>(numberArg));
      predicate.table.assign(tableSize, false);
      while (i < length)
      {
         // переменные
         bool bHasVariables = false;
         std::vector<size_t> vIdxVar(numberArg, 0);
         for (size_t iVar = 0; iVar < numberArg; ++iVar)
         {
            if (skipSpace(str_, i))
            {
               if (iVar != 0)
                  throw CException(QString("Недостаточно переменных в строке таблицы истинности у предиката \"%1\". Ожидалось %2 переменных, имеется %3.").arg(predicate.name).arg(numberArg).arg(iVar), "Ошибка добавления предиката", "CPredicatesStorage::AddPredicates");

               break;
            }

            QString var = highlightName(str_, i);

            if (var.isEmpty())
               throw CException(QString("Некорректое имя переменной в таблице истинности у предиката \"%1\".").arg(predicate.name), "Ошибка добавления предиката", "CPredicatesStorage::AddPredicates");

            auto it = m_mapVariables.find(var);
            if (it == m_mapVariables.end())
            {
               if (iVar != 0)
                  throw CException(QString("Переменной \"%1\" нет в списке переменных. Встречено в таблице истинности у предиката \"%2\".").arg(var).arg(predicate.name), "Ошибка добавления предиката", "CPredicatesStorage::AddPredicates");
               else
               {
                  nameNextPredicate = var;
                  break;
               }
            }

            vIdxVar[iVar] = it->second;
            bHasVariables = true;

            skipSpace(str_, i, ',');
         }

         if (bHasVariables)
         {
            skipSpace(str_, i, ';');

            // Всевозможные проверки таблицы
            size_t foundIndex = predicate.GetIndex(static_cast<size_t>(m_vVariables.size()), vIdxVar);
            if (foundIndex >= tableSize)
               throw CException("Ошибка индексирования! Обратитесь к разработчику.", "Ошибка добавления предиката", "CPredicatesStorage::AddPredicates");

            predicate.table[foundIndex] = true;
         }
         else
         {
            break;
         }
      }

      // добавление предиката в таблицу предикатов

      m_vPredicates.push_back(predicate);
      m_mapPredicates.emplace(predicate.name, m_vPredicates.size() - 1);
   }
}

QString CPredicatesStorage::StringVariables() const
{
   QString strVariables;

   if (m_vVariables.empty())
      return strVariables;

   for (const auto& var : m_vVariables)
      strVariables += var + ", ";

   strVariables.chop(2);

   return strVariables;
}

QString CPredicatesStorage::StringPredicatesWithTable() const
{
   QString strPredicates;

   if (m_vPredicates.empty())
      return strPredicates;

   for (size_t iPred = 0; iPred < m_vPredicates.size(); ++iPred)
   {
      QString strPred = GetPredicateName(iPred) + '(' + QString().setNum(CountArguments(iPred)) + ")";

      // таблица истинности
      CCounter<size_t> counter(0, m_vVariables.size(), std::vector<size_t>(CountArguments(iPred), 0));
      for (size_t iArg = 0; iArg < m_vPredicates.at(iPred).table.size(); ++iArg)
      {
         if (GetValuePredicate(iPred, iArg))
         {
            strPred.append(NEW_LINE);

            const std::vector<size_t>& idxsVars = counter.get();

            for (const auto& iVar : idxsVars)
               strPred += m_vVariables.at(iVar) + ", ";

            strPred.chop(2);
         }

         ++counter;
      }

      if (!strPredicates.isEmpty())
         strPredicates.append(DOUBLE_NEW_LINE);

      strPredicates.append(strPred);
   }

   return strPredicates;
}

QString CPredicatesStorage::StringPredicateWithArg(size_t indexPredicate_, size_t indexArguments_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      throw CException(INVALID_PREDICATE.arg(m_vPredicates.size()).arg(indexPredicate_), "Ошибка. Обратитесь к разработчику", "CPredicatesStorage::GetValuePredicate");

   if (indexArguments_ >= m_vPredicates.at(indexPredicate_).table.size())
      throw CException(INVALID_TABLE.arg(GetPredicateName(indexPredicate_)).arg(m_vPredicates.at(indexPredicate_).table.size()).arg(indexArguments_), "Ошибка. Обратитесь к разработчику", "CPredicatesStorage::GetValuePredicate");

   auto idxsVars = m_vPredicates.at(indexPredicate_).GetArgs(m_vVariables.size(), indexArguments_);

   if (idxsVars.empty())
      throw CException("Обратитесь к разработчику.", "Непредвиденная ошибка", "CPredicatesStorage::StringPredicateWithArg");

   QString strPred = m_vPredicates.at(indexPredicate_).name + '(';

   for (const auto& iVar : idxsVars)
      strPred += m_vVariables.at(iVar) + ", ";

   strPred.chop(2);
   strPred.append(')');

   return strPred;
}

const std::vector<QString>& CPredicatesStorage::GetVariables() const
{
   return m_vVariables;
}

size_t CPredicatesStorage::GetIndexPredicate(const QString& namePredicate_) const
{
   auto it = m_mapPredicates.find(namePredicate_);
   if (it != m_mapPredicates.end())
      return it->second;

   return SIZE_MAX;
}

size_t CPredicatesStorage::GetIndexArgument(size_t indexPredicate_, const std::vector<QString>& arguments_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      return SIZE_MAX;

   std::vector<size_t> args(arguments_.size());
   for (size_t i = 0; i < arguments_.size(); ++i)
   {
      const auto it = m_mapVariables.find(arguments_.at(i));
      if (it == m_mapVariables.end())
         return SIZE_MAX;

      args[i] = static_cast<size_t>(it->second);
   }

   return m_vPredicates.at(indexPredicate_).GetIndex(static_cast<size_t>(m_vVariables.size()), args);
}

const SPredicate& CPredicatesStorage::GetPredicate(size_t indexPredicate_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      throw CException(INVALID_PREDICATE.arg(m_vPredicates.size()).arg(indexPredicate_), "Ошибка. Обратитесь к разработчику", "CPredicatesStorage::GetPredicate");

   return m_vPredicates.at(indexPredicate_);
}

bool CPredicatesStorage::GetValuePredicate(size_t indexPredicate_, size_t indexArguments_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      throw CException(INVALID_PREDICATE.arg(m_vPredicates.size()).arg(indexPredicate_), "Ошибка. Обратитесь к разработчику", "CPredicatesStorage::GetValuePredicate");

   if (indexArguments_ >= m_vPredicates.at(indexPredicate_).table.size())
      throw CException(INVALID_TABLE.arg(GetPredicateName(indexPredicate_)).arg(m_vPredicates.at(indexPredicate_).table.size()).arg(indexArguments_), "Ошибка. Обратитесь к разработчику", "CPredicatesStorage::GetValuePredicate");

   return m_vPredicates.at(indexPredicate_).table.at(indexArguments_);
}

QString CPredicatesStorage::GetPredicateName(size_t indexPredicate_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      throw CException(INVALID_PREDICATE.arg(m_vPredicates.size()).arg(indexPredicate_), "Ошибка при получении имени предиката", "CPredicatesStorage::GetPredicateName");

   return m_vPredicates.at(indexPredicate_).name;
}

std::vector<QString> CPredicatesStorage::GetArgumentVariables(size_t indexPredicate_, size_t indexArguments_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      throw CException(INVALID_PREDICATE.arg(m_vPredicates.size()).arg(indexPredicate_), "Ошибка при получении имен переменных из таблицы истинности", "CPredicatesStorage::GetArgumentVariables");

   if (indexArguments_ >= m_vPredicates.at(indexPredicate_).table.size())
      throw CException(INVALID_TABLE.arg(GetPredicateName(indexPredicate_)).arg(m_vPredicates.at(indexPredicate_).table.size()).arg(indexArguments_), "Ошибка при получении имен переменных из таблицы истинности", "CPredicatesStorage::GetArgumentVariables");

   auto vIdxVariable = m_vPredicates.at(indexPredicate_).GetArgs(m_vVariables.size(), indexArguments_);

   if (vIdxVariable.empty())
      throw CException("Непредвиденная ошибка.", "Ошибка", "CPredicatesStorage::GetArgumentVariables");

   std::vector<QString> vNameVariable(vIdxVariable.size());

   for (size_t i = 0; i < vIdxVariable.size(); ++i)
      vNameVariable[i] = m_vVariables.at(vIdxVariable.at(i));

   return vNameVariable;
}

bool CPredicatesStorage::IsEmpty() const
{
   return m_vPredicates.empty() || m_mapPredicates.empty();
}

size_t CPredicatesStorage::CountPredicates() const
{
   return m_vPredicates.size();
}

size_t CPredicatesStorage::CountVariables() const
{
   return m_vVariables.size();
}

size_t CPredicatesStorage::CountArguments(size_t indexPredicate_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      throw CException(INVALID_PREDICATE.arg(m_vPredicates.size()).arg(indexPredicate_), "Ошибка. Обратитесь к разработчику", "CPredicatesStorage::GetCountArguments");

   return intLog(m_vVariables.size(), m_vPredicates.at(indexPredicate_).table.size());
}

void CPredicatesStorage::Clear()
{
   m_mapVariables.clear();
   m_vVariables.clear();
   m_mapPredicates.clear();
   m_vPredicates.clear();
}

bool CPredicatesStorage::isIllegalSymbol(QChar symb_)
{
   for (char ch : RESERVED_CHARACTERS)
      if (symb_ == ch)
         return true;

   return false;
}

QString CPredicatesStorage::highlightName(const QString& str_, qsizetype& index_)
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

bool skipSpace(const QString& str_, qsizetype& index_)
{
   while (index_ < str_.size() && str_.at(index_).isSpace())
      ++index_;

   return index_ >= str_.size();
}

void skipSpace(const QString& str_, qsizetype& index_, QChar skipSymbol_)
{
   if (!skipSpace(str_, index_))
      if (str_.at(index_) == skipSymbol_)
         ++index_;
}
