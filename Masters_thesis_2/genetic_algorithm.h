#pragma once
#include <QObject>
#include <QString>
#include <vector>
#include <map>

#include "random.h"
#include "predicate.h"
#include "exception.h"

// Индексы предикатов.
struct SIndexPredicate
{
   size_t idxPredicat;
   size_t idxTable;

   SIndexPredicate(size_t idxPredicat_, size_t idxTable_) : idxPredicat(idxPredicat_), idxTable(idxTable_) {}
};

// Условие целостности (одно).
struct SCondition
{
   std::vector<SIndexPredicate> left; // левая часть (X -> ...)
   std::vector<SIndexPredicate> right; // правая часть (... -> X)

   inline size_t GeneralCount() const { return left.size() + right.size(); }
};

class QTextStream;

enum EAddingError
{
   eSuccessfully,       // Успешное добавление переменной
   eUnknownVariable,    // Неизвестная переменная
   VariablePresent,     // Переменная уже присутствует
   OppositeCondition,   // Переменная используется в другой части условия
   eInvalidIndex        // Неверный индекс
};

class CError
{
   QString m_title;
   QString m_errorMessage;
};

class CGeneticAlgorithm : public QObject
{
   Q_OBJECT

   using TIntegrityLimitation = std::vector<SCondition>; // Ограничение целостности (вектор условий).

   CPredicates m_predicates; // Предикаты (там же хранятся и переменные)
   TIntegrityLimitation m_original; // Изначальное ограничение целостности (для финтес ф-ции)
   std::vector<TIntegrityLimitation> m_vGenerations;

   mutable CRandom m_rand;

public:
   CGeneticAlgorithm();
   ~CGeneticAlgorithm() = default;

   // Получает данные из файла
   // !> emit signal error
   void FillDataInFile(const QString& fileName_);

   // Возвращает строку с переменными.
   QString StringVariables() const;

   // Возвращает строку с предикатами.
   QString StringPredicates() const;

   // Возвращает строку с ограничением целостности.
   QString StringIntegrityLimitation() const;

   // Возвращает строку с поклоениями. Выводит первые count_ особей.
   // Чтобы вывести все поколения оставьте значение по умолчанию.
   QString StringGeneration(size_t count_ = SIZE_MAX) const;

   // Возвращает настраиваемую строку.
   // fileName_ - имя файла.
   // bVariables_ - Записать переменные.
   // bPredicates_ - Записать предикаты и их таблицы истинности.
   // bIntegrityLimitation_ - Записать исходное ограничение целостности.
   // bGenerations_ - Записать поколения.
   // countIndividuals_ - Количество первых особей.
   QString StringCustom(bool bVariables_ = true, bool bPredicates_ = true, bool bIntegrityLimitation_ = true, bool bGeneration_ = true, size_t countIndividuals_ = SIZE_MAX) const;

   // Запись в файл.
   // fileName_ - имя файла.
   // bVariables_ - Записать переменные.
   // bPredicates_ - Записать предикаты и их таблицы истинности.
   // bIntegrityLimitation_ - Записать исходное ограничение целостности.
   // bGenerations_ - Записать поколения.
   // countIndividuals_ - Количество первых особей.
   // !> emit signal error
   void WriteInFile(const QString& fileName_, bool bVariables_ = true, bool bPredicates_ = true, bool bIntegrityLimitation_ = true, bool bGeneration_ = true, size_t countIndividuals_ = SIZE_MAX) const;

   // Запустить алгоритм с заданием параметров.
   // countIndividuals_ - количество особей
   // countIterations_ - количество итераций
   // percentMutationArguments_ - процент мутаций аргументов в предикате в одном условии
   // countSkipMutationArg_ - количество последних итераций которых не затронет мутация аргументов
   // percentMutationPredicates_ - процент мутаций предикатов (полностью, с аргументами) в одном условии
   // countSkipMutationPred_ - количество последних итераций которых не затронет мутация предикатов
   // percentIndividualsUndergoingMutation_ - процент особей которые будут подвергнуты мутациям (в каждом поколении)
   void Start(int countIndividuals_, int countIterations_, double percentMutationArguments_, int countSkipMutationArg_, double percentMutationPredicates_, int countSkipMutationPred_, double percentIndividualsUndergoingMutation_ = 100);

   void Clear();

   bool HasGenerations() const;

   static bool isIllegalSymbol(QChar symbol_);

signals:
   void signalProgressUpdate(int value_) const;
   void signalEnd() const;
   void signalError(const CException& error_) const;


private:

   // Считывает и заносит условия из строки.
   void SetConditionsFromString(const QString& str_);

   // Записывает условие в строку.
   QString StringCondition(const SCondition& condition_) const;

   // Записывает ограничение целостности в строку.
   QString StringIntegrityLimitation(const TIntegrityLimitation& integrityLimitation_, bool bInsertNewLine_ = false) const;

   // Сгенерировать первое поколение рандомно.
   void CreateFirstGenerationRandom(size_t count_);

   // Скрещивание только по предикатам.
   TIntegrityLimitation CrossingOnlyPredicates(const TIntegrityLimitation& parent1_, const TIntegrityLimitation& parent2_) const;

   // Селекция. Выбираются лучшие (по фитнесс функции) CountSurvivors_ особей из Individuals_, т.е. полная замена, родителей "убиваем".
   void Selection(const std::vector<TIntegrityLimitation>& individual_, size_t countSurvivors_);

   // Мутация аргументов в предикате.
   void MutationArguments(TIntegrityLimitation& individual_, double ratio_) const;

   // Мутация предикатов.
   void MutationPredicates(TIntegrityLimitation& individual_, double ratio_) const;

   // Возвращает количество всех предикатов во всем ограничении.
   size_t CountAllPreficates(const TIntegrityLimitation& individual_) const;

   bool IsCorrectCondition(const SCondition& cond_) const;

   bool IsTrueCondition(const SCondition& cond_) const;

   // Фитнес функци.
   // Считает общее кол-во предикатов (N) в условии целостности.
   // Значение для предиката равно P / N, где P = [0, 1] - корректность предиката.
   // Если предикат изменен вообще (другой предикат) то P = 0,
   // если предикат тот же, но изменились его аргументы, то корректность P = (1 - arg / cha),
   // где arg - количество аргументов у предиката, а cha - количество отличных от изначального условия аргументов.
   // В итоге, если условие целостности считается истинным, то общий результат будет помножен на 2.
   double FitnessFunction(const TIntegrityLimitation& conds_) const;

   size_t SelectRandParent(size_t countIndividuals_) const;

   std::pair<size_t, size_t> GetPairParents(size_t countIndividuals_) const;

   void SortDescendingOrder();

   void SortRestriction(TIntegrityLimitation& individual_) const;

   static QString highlightBlock(const QString& str_, qsizetype& index_);
   static QString highlightName(const QString& str_, qsizetype& index_);
   static bool hasSymbol(const QString str_, qsizetype& index_, QChar symbol_);

   SIndexPredicate getPredicate(const QString& str_, qsizetype& index_) const;
};