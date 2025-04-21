#include "parser_template_predicates.h"
#include "exception.h"


// Возвращает true и увеличивает index_ на 1, если в позиции index_ есть символ symbol_,
// иначе возвращает false.
static bool hasSymbol(const QString& str_, qsizetype& index_, QChar symbol_)
{
   if (index_ < str_.size() && str_.at(index_) == symbol_)
   {
      ++index_;
      return true;
   }

   return false;
}

// Возвращает true, если symbol_ является зарезервированным символом, false - иначе.
static bool isIllegalSymbol(QChar symbol_)
{
   const QChar illegalSymbols[] = { ',', '-','>', '$', '(', ')', SYMBOL_COMPLETION_CONDEITION };

   for (const auto& illegal : illegalSymbols)
      if (symbol_ == illegal)
         return true;

   return false;
}

// Возвращает имя из строки str_ начиная с позиции index_.
// После успешного считывания имени index_ указывает на следующий символ после имени.
static QString highlightName(const QString& str_, qsizetype& index_)
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

// =-=-=-=-=-=-=-=-=-=-=-=-=-= Методы класса =-=-=-=-=-=-=-=-=-=-=-=-=-=

CParserTemplatePredicates::CParserTemplatePredicates(const CPredicatesStorage* storage_) :
	m_storage(storage_)
{
	if (!m_storage)
		throw CException("Нет предикатов!");
}

SCondition CParserTemplatePredicates::Parse(const QString& condition_) const
{
   SCondition result;
   std::map<QString, int> mapTemplateArgs;

   const qsizetype length = condition_.length();
   qsizetype i = 0;

   // считываем левую часть условия
   while (!skipSpace(condition_, i))
   {
      if (hasSymbol(condition_, i, '-'))
      {
         if (hasSymbol(condition_, i, '>'))
            break; // выход из цикла по левой части условия
         else
            throw CException("После символа \'-\' ожидался символ \'>\'.");
      }

      result.left.push_back(getPredicate(condition_, i, mapTemplateArgs, result.maxArgument));
   }

   if (result.left.empty())
      throw CException("Отсутствует левая часть условия.");

   // считываем правую часть условия
   while (!skipSpace(condition_, i))
   {
      if (hasSymbol(condition_, i, ';'))
         break; // выход из цикла по правой части условия

      result.right.push_back(getPredicate(condition_, i, mapTemplateArgs, result.maxArgument));
   }

   if (result.right.empty())
      throw CException("Отсутствует правая часть условия.");

   return result;
}

SPredicateTemplate CParserTemplatePredicates::getPredicate(const QString& str_, qsizetype& index_, std::map<QString, int>& mapTemplateArgs_, int& maxArg_) const
{
   QString predicateName = highlightName(str_, index_);
   if (predicateName.isEmpty())
      throw CException(QString("Некорректное имя предиката! Встречено:\n%1").arg(str_.mid(index_)));

   size_t idxPredicate = m_storage->GetIndexPredicate(predicateName);
   if (idxPredicate == SIZE_MAX)
      throw CException(QString("Не найден предикат с именем \"%1\"").arg(predicateName));

   skipSpace(str_, index_);
   if (!hasSymbol(str_, index_, '('))
      throw CException(QString("После имени предиката \"%1\" ожидаось \'(\'.").arg(predicateName));

   size_t countArg = m_storage->CountArguments(idxPredicate);
   std::vector<int> vArguments(countArg);
   for (quint16 iArg = 0; iArg < countArg; ++iArg)
   {
      if (skipSpace(str_, index_))
         throw CException(QString("Недостаточно аргументов у предиката \"%1\".").arg(predicateName));

      QString nameArg = highlightName(str_, index_);
      if (nameArg.isEmpty())
         throw CException(QString("Некорректное имя переменной у аргумента предиката \"%1\"").arg(predicateName));

      auto itTemplArg = mapTemplateArgs_.find(nameArg);
      if (itTemplArg == mapTemplateArgs_.end())
         itTemplArg = mapTemplateArgs_.emplace(nameArg, ++maxArg_).first;

      vArguments[iArg] = itTemplArg->second;

      skipSpace(str_, index_, ',');
   }

   if (!hasSymbol(str_, index_, ')'))
      throw CException(QString("Ожидалось \')\' у предиката \"%1\".").arg(predicateName));

   return SPredicateTemplate(idxPredicate, vArguments);
}

QString CParserTemplatePredicates::GetStringTemplatePredicate(const SPredicateTemplate& predicate_) const
{
   QString result = m_storage->GetPredicateName(predicate_.idxPredicate) + '(';

   for (int val : predicate_.arguments)
      result.append(GetLetterDesignationByNumber(val) + ", ");

   result.chop(2);
   result.append(')');
   return result;
}

QString CParserTemplatePredicates::GetLetterDesignationByNumber(int value_)
{
   const int number = value_ / 26;
   const char letter = 'a' + value_ % 26;
   QString result(letter);
   if (number > 0)
      result.append(QString::number(number));

   return result;
}

void SCondition::ForEachPredicate(std::function<void(SPredicateTemplate&)> callback_)
{
   for (SPredicateTemplate& predTempl : left)
      callback_(predTempl);

   for (SPredicateTemplate& predTempl : right)
      callback_(predTempl);
}

void SCondition::ForEachPredicate(std::function<void(const SPredicateTemplate&)> callback_) const
{
   for (const SPredicateTemplate& predTempl : left)
      callback_(predTempl);

   for (const SPredicateTemplate& predTempl : right)
      callback_(predTempl);
}

void SCondition::ForEachArgument(std::function<void(int&)> callback_)
{
   ForEachPredicate([&callback_](SPredicateTemplate& predTempl)
      {
         for (int& arg : predTempl.arguments)
            callback_(arg);
      });
}

void SCondition::ForEachArgument(std::function<void(int)> callback_) const
{
   ForEachPredicate([&callback_](const SPredicateTemplate& predTempl)
      {
         for (int arg : predTempl.arguments)
            callback_(arg);
      });
}

void SCondition::RecalculateMaximum()
{
   ForEachArgument([&](int arg)
      {
         if (arg > maxArgument)
            maxArgument = arg;
      });
}
