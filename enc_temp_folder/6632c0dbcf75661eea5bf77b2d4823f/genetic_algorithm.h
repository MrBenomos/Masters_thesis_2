#pragma once
#include <vector>
#include <tuple>

#include <QObject>
#include <QString>

#include "random.h"
#include "predicate.h"
#include "exception.h"

class QTextStream;

class CGeneticAlgorithm : public QObject
{
   Q_OBJECT

   // ================================ С т р у к т у р ы ================================

   // Экземпляр предиката.
   struct SPredicateInstance
   {
      size_t idxPredicate; // индекс предиката
      size_t idxTable;     // индекс из таблицы истинности для набора аргументов

      SPredicateInstance(size_t idxPredicat_, size_t idxTable_) : idxPredicate(idxPredicat_), idxTable(idxTable_) {}
   };

   using TPartCondition = std::vector<SPredicateInstance>;

   // Условие целостности (одно).
   struct SCondition
   {
      TPartCondition left; // левая часть (X -> ...)
      TPartCondition right; // правая часть (... -> X)

      inline size_t GeneralCount() const { return left.size() + right.size(); }
   };

   // Для фитнес функции - количества.
   struct SCounts
   {
      // Аргументы
      size_t diffArg = 0;     // Несовпадающие
      size_t totalArg = 0;    // Всего
      // Предикаты
      size_t matchPred = 0;   // Совпадающие
      size_t addedPred = 0;   // Добавленые
      size_t deletedPred = 0; // Удаленные

      SCounts& operator+=(const SCounts& added_);
   };

   using TIntegrityLimitation = std::vector<SCondition>; // Ограничение целостности (вектор условий).
   using TIntLimAndFitness = std::pair<TIntegrityLimitation, double>; // Ограничение целостности и фитнес.
   using TGeneration = std::vector<TIntLimAndFitness>; // Поколение - вектор ограничений с фитнесом.

   // =============================== П е р е м е н н ы е ===============================

   // Предикаты (там же хранятся и переменные).
   CPredicates m_predicates;

   // Изначальное ограничение целостности (для финтес ф-ции). 
   TIntegrityLimitation m_original;

   // Одно поколение (состоящее из множества особей/индивидуумов).
   TGeneration m_generation;

   mutable CRandom m_rand;

   // Нижняя граница для измененных аргументов в предикате.
   // Если все аргументы отличаются стоимость предиката будет такая.
   double m_minCostForArgDif = 0.75;

   // Цена добавления предиката [0; 1].
   double m_costAddingPredicate = 0.5;

public:
   // ========================== О т к р ы т ы е   м е т о д ы ==========================
   CGeneticAlgorithm();
   ~CGeneticAlgorithm() = default;

   // Получает данные из файла.
   // !> emit signal error.
   void FillDataInFile(const QString& fileName_);

   // Возвращает строку с переменными.
   QString StringVariables() const;

   // Возвращает строку с предикатами.
   QString StringPredicates() const;

   // Возвращает строку с ограничением целостности.
   QString StringIntegrityLimitation() const;

   // Возвращает строку с поклоениями. Выводит первые count_ особей.
   // Чтобы вывести все поколения оставьте значение по умолчанию.
   QString StringGeneration(bool bFitness_ = true, size_t count_ = SIZE_MAX) const;

   // Возвращает настраиваемую строку.
   // fileName_ - имя файла.
   // bVariables_ - Записать переменные.
   // bPredicates_ - Записать предикаты и их таблицы истинности.
   // bIntegrityLimitation_ - Записать исходное ограничение целостности.
   // bGenerations_ - Записать поколения.
   // bFitness - Выводить значение фитнес функции.
   // countIndividuals_ - Количество первых особей.
   QString StringCustom(bool bVariables_ = true, bool bPredicates_ = true, bool bIntegrityLimitation_ = true, bool bGeneration_ = true, bool bFitness_ = true, size_t countIndividuals_ = SIZE_MAX) const;

   // Запись в файл.
   // fileName_ - имя файла.
   // bVariables_ - Записать переменные.
   // bPredicates_ - Записать предикаты и их таблицы истинности.
   // bIntegrityLimitation_ - Записать исходное ограничение целостности.
   // bGenerations_ - Записать поколения.
   // countIndividuals_ - Количество первых особей.
   // !> emit signal error.
   void WriteInFile(const QString& fileName_, bool bVariables_ = true, bool bPredicates_ = true, bool bIntegrityLimitation_ = true, bool bGeneration_ = true, bool bFitness_ = true, size_t countIndividuals_ = SIZE_MAX) const;

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

   // Устанавливает цену нижней границы для измененных аргументов.
   // Допустимые значения в интевале (0; 1).
   // !> emit signal error.
   void SetLimitOfArgumentsChange(double value_);

   // Устанавливает цену добавления нового предиката.
   // Допустимые значения в отрезке [0; 1].
   // !> emit signal error.
   void SetCostAddingPredicate(double cost_);

   static bool isIllegalSymbol(QChar symbol_);

signals:
   // ================================== С и г н а л ы ==================================
   void signalProgressUpdate(int value_) const;
   void signalEnd() const;
   void signalError(const CException& error_) const;


private:
   //  ========================= З а к р ы т ы е   м е т о д ы ==========================

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

   // Селекция. Выбираются лучшие (по фитнесс функции) CountSurvivors_ особей из individuals_, т.е. полная замена, родителей "убиваем".
   void Selection(const TGeneration& individuals_, size_t countSurvivors_);

   // Мутация аргументов в предикате.
   void MutationArguments(TIntegrityLimitation& individual_, double ratio_) const;

   // Мутация предикатов.
   void MutationPredicates(TIntegrityLimitation& individual_, double ratio_) const;

   // Возвращает количество всех предикатов во всем ограничении.
   size_t CountAllPredicates(const TIntegrityLimitation& individual_) const;

   // Возвращает корректность условия (обе части должны быть не пусты).
   bool IsCorrectCondition(const SCondition& cond_) const;

   // Возвращает истинность условия.
   bool IsTrueCondition(const SCondition& cond_) const;

   // Возвращает индекс родителя из поколения. Турнирная функция выбора.
   size_t SelectRandParent(size_t countIndividuals_) const;

   // Возвращает индексы двух разных родителей из поколения.
   // Использует турнирный отбор.
   std::pair<size_t, size_t> GetPairParents(size_t countIndividuals_) const;

   // Сортирует поколение в порядке убывания фитнес функции.
   void SortGenerationDescendingOrder();

   // Фитнес функция.
   // Возвращает значение приспособленности (фитнеса) для ограничения целостности.
   // Считается отдельно для каждого условия FC - фитнес одного условия.
   // Конечное значение это сумма всех FC.
   // Расчет для условия:
   // Считается кол-во предикатов (N) в одном условии целостности.
   // Значение для предиката равно P / N, где P = [0, 1] - цена предиката.
   // Цену добавления предиката устанавливает пользователь P = (m_costAddingPredicate).
   // Цена удаления предиката равна P = 0.
   // Цена предиката который есть в оригинальном условии вычисляется так:
   // Все аргументы совпали - P = 1.
   // Все аргументы изменились - P = нижняя_граница (устанавливается пользователем).
   // dif аргуметнов поменялось из tot то P = нижняя_граница + ((tot-dif)/tot) * (1 - нижняя_граница).
   double FitnessFunction(const TIntegrityLimitation& conds_) const;

   // ----------------------- Вспомогательные функции для фитнеса -----------------------

   // Возвращает индексы предикатов в условии, которые имеют такой же индекс как и у sample_ в порядке возрастания отличий в аргументах.
   // Возвращает multimap<число_отличий, индекс_предиката>.
   std::multimap<int, size_t> findMinDifference(const SPredicateInstance& sample_, const TPartCondition& verifiable_) const;

   // Количественная оценка.
   // Возвращает:
   // 1. Количество отличных аргументов у (всех) одинаковых предикат.
   // 2. Количество всего аргументов у этих предикат (всех).
   // 3. Количество совпадающих предикат.
   // 4. Количество добавленных предикат.
   // 5. Количество удаленных предикат.
   SCounts quantitativeAssessment(const TPartCondition& sample_, const TPartCondition& verifiable_) const;

   // Возвращает множитель аргументов.
   double getMultiplierArguments(size_t differences_, size_t total_) const;

   // --------------------------- Функции работы со строками ----------------------------

   // Получает экземпляр предиката из строки (с аргументами).
   SPredicateInstance getPredicate(const QString& str_, qsizetype& index_) const;

   static QString highlightBlock(const QString& str_, qsizetype& index_);
   static QString highlightName(const QString& str_, qsizetype& index_);
   static bool hasSymbol(const QString str_, qsizetype& index_, QChar symbol_);
};