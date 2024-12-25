#include <cmath>

#include <QTextStream>

#include "predicate.h"
#include "exception.h"
#include "counter.h"
#include "global.h"

// Неверный индекс предиката. #1 - кол-во предикатов, #2 - индекс к которому пытались обратиться.
static const QString INVALID_PREDICATE("Попытка обращения к предикату с несуществующим индексом. Всего предикатов: %1, попытка обращения к: %2");
// Неверный индекс таблицы истинности. #1 - Имя предиката, #2 - кол-во строк в таблице, #3 - индекс к которому пытались обратиться.
static const QString INVALID_TABLE("Попытка обращения к таблице истинности с несуществующим индексом у предиката \"%1\". Всего строк: %2, попытка обращения к: %3");
// Ошибка: невозможно изменить переменные.
static const CException EXC_CANT_ADD_VARIABLE("Невозможно изменить набор переменных, после добавления хотя бы одного предиката.", "Ошибка изменения переменых", "CPredicates::SetVariables");

constexpr const char RESERVED_CHARACTERS[] = "(),=";

inline size_t pow(size_t base_, size_t exp_)
{
   size_t result = 1;
   while (exp_ > 0)
   {
      if (exp_ % 2 == 1)
         result *= base_;

      base_ *= base_;
      exp_ /= 2;
   }

   return result;
}

inline size_t intLog(size_t base_, size_t x_)
{
   return static_cast<size_t>(log(x_) / log(base_) + 0.01);
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

void CPredicates::SetVariables(const std::set<QString>& variables_)
{
   if (!m_mapPredicates.empty() || !m_vPredicates.empty())
      throw EXC_CANT_ADD_VARIABLE;


   m_vVariables.resize(variables_.size());

   size_t idx = 0;
   for (const auto& var : variables_)
   {
      if (var.isEmpty())
         throw CException("Некорректное имя переменной.", "Ошибка изменения переменых", "CPredicates::SetVariables");

      m_vVariables[idx] = var;
      m_mapVariables.emplace(var, idx++);
   }
}

bool CPredicates::SetVariables(const std::vector<QString>& variables_)
{
   if (!m_mapPredicates.empty() || !m_vPredicates.empty())
      throw EXC_CANT_ADD_VARIABLE;

   std::set<QString> setName;
   for (const auto& var : variables_)
      setName.emplace(var);

   SetVariables(setName);

   return variables_.size() == m_vVariables.size();
}

void CPredicates::SetVariables(const QString& str_)
{
   if (!m_mapPredicates.empty() || !m_vPredicates.empty())
      throw EXC_CANT_ADD_VARIABLE;

   std::set<QString> setName;

   for (qsizetype i = 0; i < str_.size(); /*++i*/)
   {
      skipSpace(str_, i);
      QString name = highlightName(str_, i);

      if (name.isEmpty())
         throw CException(QString("Ожидалось имя переменной. Встречен символ \'%1\'").arg(str_.at(i)), "Ошибка изменения переменых", "CPredicates::SetVariables");

      setName.emplace(name);
      skipSpace(str_, i, ',');
   }

   SetVariables(setName);
}

void CPredicates::AddPredicates(const QString& str_)
{
   if (m_vVariables.empty() || m_mapVariables.empty())
      throw CException("Невозможно создать предикат, список всех переменных пуст!", "Ошибка добавления предиката", "CPredicates::AddPredicates");

   qsizetype length = str_.size();
   // Цикл по предикатам.
   for (qsizetype i = 0; i < length; /*++i*/)
   {
      SPredicate predicate;

      if (skipSpace(str_, i))
         break;

      // имя предиката

      predicate.name = highlightName(str_, i);
      if (predicate.name.isEmpty())
         throw CException("Некорректное имя предиката.", "Ошибка добавления предиката", "CPredicates::AddPredicates");

      // проверка существования
      auto it = m_mapPredicates.find(predicate.name);
      if (it != m_mapPredicates.end())
         throw CException(QString("Попытка добавить предикат с уже существующим именем \"%1\".").arg(predicate.name), "Ошибка добавления предиката", "CPredicates::AddPredicates");

      skipSpace(str_, i, '(');

      if (skipSpace(str_, i))
         throw CException(QString("Предикат \"%1\" не закончен. Нет количества аргументов и таблицы истинности.").arg(predicate.name), "Ошибка добавления предиката", "CPredicates::AddPredicates");

      // число (кол-во аргументов)

      qint16 numberArg = 0;

      while (i < length && str_.at(i).isDigit())
      {
         numberArg *= 10;
         numberArg += str_.at(i).digitValue();
         ++i;
      }

      if (numberArg == 0)
         throw CException(QString("Ожидалось количество аргументов у предиката \"%1\". У предиката должо быть не менее 1 аргумента.").arg(predicate.name), "Ошибка добавления предиката", "CPredicates::AddPredicates");

      skipSpace(str_, i, ')');

      // таблица истинности
      size_t tableSize = pow(m_vVariables.size(), static_cast<size_t>(numberArg));
      size_t delta = 0; // используется, если в таблице несколько условий с одинаковыми переменными
      predicate.table.resize(tableSize);
      std::vector<bool> vTableFill(tableSize, false);
      for (size_t iRow = 0; iRow < tableSize + delta; ++iRow)
      {
         if (skipSpace(str_, i))
            throw CException(QString("У предиката \"%1\" не закончена таблица истинности (или отсутствует). Ожидалось %2 выражений, имеется %3.").arg(predicate.name).arg(tableSize).arg(iRow), "Ошибка добавления предиката", "CPredicates::AddPredicates");

         // переменные
         std::vector<size_t> vIdxVar(numberArg, 0);
         for (size_t iVar = 0; iVar < numberArg; ++iVar)
         {
            if (skipSpace(str_, i))
               throw CException(QString("Недостаточно переменных в строке таблицы истинности у предиката \"%1\". Ожидалось %2 переменных, имеется %3 в строке %4.").arg(predicate.name).arg(numberArg).arg(iVar).arg(iRow + delta + 1), "Ошибка добавления предиката", "CPredicates::AddPredicates");

            QString var = highlightName(str_, i);

            if (var.isEmpty())
               throw CException(QString("Некорректое имя переменной в таблице истинности у предиката \"%1\" в строке %2.").arg(predicate.name).arg(iRow + delta + 1), "Ошибка добавления предиката", "CPredicates::AddPredicates");

            auto it = m_mapVariables.find(var);
            if (it == m_mapVariables.end())
               throw CException(QString("Переменной \"%1\" нет в списке переменных. Встречено в таблице истинности у предиката \"%2\" в строке %3.").arg(var).arg(predicate.name).arg(iRow + delta + 1), "Ошибка добавления предиката", "CPredicates::AddPredicates");

            vIdxVar[iVar] = it->second;

            skipSpace(str_, i, ',');
         }

         // значение предиката для данных аргументов

         skipSpace(str_, i, '=');
         skipSpace(str_, i);

         bool value;

         if (i < length)
         {
            if (str_.at(i) == '0')
               value = false;
            else if (str_.at(i) == '1')
               value = true;
            else
               throw CException(QString("Некорректное значение предиката \"%1\" в строке %2. Ожидалось 0 или 1.").arg(predicate.name).arg(iRow + delta + 1), "Ошибка добавления предиката", "CPredicates::AddPredicates");
         }
         else
            throw CException(QString("Ожидалось значение предиката \"%1\" в строке %2.").arg(predicate.name).arg(iRow + delta + 1), "Ошибка добавления предиката", "CPredicates::AddPredicates");

         ++i;

         skipSpace(str_, i, ',');


         // Всевозможные проверки таблицы
         size_t foundIndex = predicate.GetIndex(static_cast<size_t>(m_vVariables.size()), vIdxVar);
         if (foundIndex >= tableSize)
            throw CException("Ошибка индексирования! Обратитесь к разработчику.", "Ошибка добавления предиката", "CPredicates::AddPredicates");

         if (vTableFill.at(foundIndex))
         {
            if (predicate.table.at(foundIndex) != value)
               throw CException(QString("Обнаружена попытка переопределить значение для одних и тех же аргументов в предикате \"%1\" в строке %2.").arg(predicate.name).arg(iRow + delta + 1), "Ошибка добавления предиката", "CPredicates::AddPredicates");

            ++delta;
         }
         else
         {
            vTableFill[foundIndex] = true;
            predicate.table[foundIndex] = value;
         }
      }

      // добавление предиката в таблицу предикатов

      m_vPredicates.push_back(predicate);
      m_mapPredicates.emplace(predicate.name, m_vPredicates.size() - 1);
   }
}

QString CPredicates::StringVariables() const
{
   QString strVariables;

   if (m_vVariables.empty())
      return strVariables;

   for (const auto& var : m_vVariables)
      strVariables += var + ", ";

   strVariables.chop(2);

   return strVariables;
}

QString CPredicates::StringPredicatesWithTable() const
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
         strPred.append(NEW_LINE);

         const std::vector<size_t>& idxsVars = counter.data();

         for (const auto& iVar : idxsVars)
            strPred += m_vVariables.at(iVar) + ", ";

         strPred.chop(2);

         if (GetValuePredicate(iPred, iArg))
            strPred.append("= 1");
         else
            strPred.append("= 0");

         ++counter;
      }

      if (!strPredicates.isEmpty())
         strPredicates.append(DOUBLE_NEW_LINE);

      strPredicates.append(strPred);
   }

   return strPredicates;
}

QString CPredicates::StringPredicateWithArg(size_t indexPredicate_, size_t indexArguments_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      throw CException(INVALID_PREDICATE.arg(m_vPredicates.size()).arg(indexPredicate_), "Ошибка. Обратитесь к разработчику", "CPredicates::GetValuePredicate");

   if (indexArguments_ >= m_vPredicates.at(indexPredicate_).table.size())
      throw CException(INVALID_TABLE.arg(GetPredicateName(indexPredicate_)).arg(m_vPredicates.at(indexPredicate_).table.size()).arg(indexArguments_), "Ошибка. Обратитесь к разработчику", "CPredicates::GetValuePredicate");

   auto idxsVars = m_vPredicates.at(indexPredicate_).GetArgs(m_vVariables.size(), indexArguments_);

   if (idxsVars.empty())
      throw CException("Обратитесь к разработчику.", "Непредвиденная ошибка", "CPredicates::StringPredicateWithArg");

   QString strPred = m_vPredicates.at(indexPredicate_).name + '(';

   for (const auto& iVar : idxsVars)
      strPred += m_vVariables.at(iVar) + ", ";

   strPred.chop(2);
   strPred.append(')');

   return strPred;
}

const std::vector<QString>& CPredicates::GetVariables() const
{
   return m_vVariables;
}

size_t CPredicates::GetIndexPredicate(const QString& namePredicate_) const
{
   auto it = m_mapPredicates.find(namePredicate_);
   if (it != m_mapPredicates.end())
      return it->second;

   return SIZE_MAX;
}

size_t CPredicates::GetIndexArgument(size_t indexPredicate_, const std::vector<QString>& arguments_) const
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

const SPredicate& CPredicates::GetPredicate(size_t indexPredicate_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      throw CException(INVALID_PREDICATE.arg(m_vPredicates.size()).arg(indexPredicate_), "Ошибка. Обратитесь к разработчику", "CPredicates::GetPredicate");

   return m_vPredicates.at(indexPredicate_);
}

bool CPredicates::GetValuePredicate(size_t indexPredicate_, size_t indexArguments_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      throw CException(INVALID_PREDICATE.arg(m_vPredicates.size()).arg(indexPredicate_), "Ошибка. Обратитесь к разработчику", "CPredicates::GetValuePredicate");

   if (indexArguments_ >= m_vPredicates.at(indexPredicate_).table.size())
      throw CException(INVALID_TABLE.arg(GetPredicateName(indexPredicate_)).arg(m_vPredicates.at(indexPredicate_).table.size()).arg(indexArguments_), "Ошибка. Обратитесь к разработчику", "CPredicates::GetValuePredicate");

   return m_vPredicates.at(indexPredicate_).table.at(indexArguments_);
}

QString CPredicates::GetPredicateName(size_t indexPredicate_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      throw CException(INVALID_PREDICATE.arg(m_vPredicates.size()).arg(indexPredicate_), "Ошибка при получении имени предиката", "CPredicates::GetPredicateName");

   return m_vPredicates.at(indexPredicate_).name;
}

std::vector<QString> CPredicates::GetArgumentVariables(size_t indexPredicate_, size_t indexArguments_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      throw CException(INVALID_PREDICATE.arg(m_vPredicates.size()).arg(indexPredicate_), "Ошибка при получении имен переменных из таблицы истинности", "CPredicates::GetArgumentVariables");

   if (indexArguments_ >= m_vPredicates.at(indexPredicate_).table.size())
      throw CException(INVALID_TABLE.arg(GetPredicateName(indexPredicate_)).arg(m_vPredicates.at(indexPredicate_).table.size()).arg(indexArguments_), "Ошибка при получении имен переменных из таблицы истинности", "CPredicates::GetArgumentVariables");

   auto vIdxVariable = m_vPredicates.at(indexPredicate_).GetArgs(m_vVariables.size(), indexArguments_);

   if (vIdxVariable.empty())
      throw CException("Непредвиденная ошибка.", "Ошибка", "CPredicates::GetArgumentVariables");

   std::vector<QString> vNameVariable(vIdxVariable.size());

   for (size_t i = 0; i < vIdxVariable.size(); ++i)
      vNameVariable[i] = m_vVariables.at(vIdxVariable.at(i));

   return vNameVariable;
}

bool CPredicates::IsEmpty() const
{
   return m_vPredicates.empty() || m_mapPredicates.empty();
}

size_t CPredicates::CountPredicates() const
{
   return m_vPredicates.size();
}

size_t CPredicates::CountVariables() const
{
   return m_vVariables.size();
}

size_t CPredicates::CountArguments(size_t indexPredicate_) const
{
   if (indexPredicate_ >= m_vPredicates.size())
      throw CException(INVALID_PREDICATE.arg(m_vPredicates.size()).arg(indexPredicate_), "Ошибка. Обратитесь к разработчику", "CPredicates::GetCountArguments");

   return intLog(m_vVariables.size(), m_vPredicates.at(indexPredicate_).table.size());
}

void CPredicates::Clear()
{
   m_mapVariables.clear();
   m_vVariables.clear();
   m_mapPredicates.clear();
   m_vPredicates.clear();
}

bool CPredicates::isIllegalSymbol(QChar symb_)
{
   for (char ch : RESERVED_CHARACTERS)
      if (symb_ == ch)
         return true;

   return false;
}

QString CPredicates::highlightName(const QString& str_, qsizetype& index_)
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
