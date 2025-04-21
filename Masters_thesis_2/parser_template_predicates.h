#pragma once
#include <functional>

#include "predicate.h"

// Символ конца условия
constexpr const char SYMBOL_COMPLETION_CONDEITION = ';';

// Шаблон предиката.
struct SPredicateTemplate
{
   size_t idxPredicate = SIZE_MAX; // индекс предиката в хранилище
   std::vector<int> arguments; // аргументы

   SPredicateTemplate() = default;
   SPredicateTemplate(size_t idxPredicate_, const std::vector<int> arguments_) :
      idxPredicate(idxPredicate_), arguments(arguments_) {}
};

using TPartCondition = std::vector<SPredicateTemplate>;

// Условие целостности (одно).
struct SCondition
{
   TPartCondition left; // левая часть (X -> ...)
   TPartCondition right; // правая часть (... -> X)
   int maxArgument = -1; // максимальный аргумент

   inline size_t CountPredicates() const { return left.size() + right.size(); }
   void ForEachPredicate(std::function<void(SPredicateTemplate&)> callback_);
   void ForEachPredicate(std::function<void(const SPredicateTemplate&)> callback_) const;
   void ForEachArgument(std::function<void(int&)> callback_);
   void ForEachArgument(std::function<void(int)> callback_) const;

   void RecalculateMaximum();
};

class CParserTemplatePredicates
{
   const CPredicatesStorage* m_storage;

public:

   CParserTemplatePredicates(const CPredicatesStorage* storage_);

   SCondition Parse(const QString& condition_) const;

   QString GetStringTemplatePredicate(const SPredicateTemplate& predicate_) const;

   static QString GetLetterDesignationByNumber(int value_);

private:

   SPredicateTemplate getPredicate(const QString& str_, qsizetype& index_, std::map<QString, int>& mapTemplateArgs_, int& maxArg_) const;
};