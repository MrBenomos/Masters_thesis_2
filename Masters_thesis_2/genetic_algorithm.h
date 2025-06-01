#pragma once
#include <vector>
#include <tuple>

#include <QObject>
#include <QString>

#include "random.h"
#include "predicate.h"
#include "parser_template_predicates.h"
#include "sparse_truth_table.h" // Нужно удалить отсюда, если переделю хранение таблиц в предекате. %%%%%

class QTextStream;
class CException;

class CGeneticAlgorithm : public QObject
{
   Q_OBJECT

   // ================================ С т р у к т у р ы ================================

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
   CPredicatesStorage m_storage;

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

   // Использовать ускоренное схождение условий.
   bool m_boost = false;

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
   QString StringIntegrityLimitation(bool bTrueCondition_ = false, bool bUsefulnessCondition_ = false) const;

   // Возвращает строку с поклоениями. Выводит первые count_ особей.
   // Чтобы вывести все поколения оставьте значение по умолчанию.
   QString StringGeneration(bool bFitness_ = true, bool bTrueCondition_ = false, bool bUsefulnessCondition_ = false, size_t count_ = SIZE_MAX) const;

   // Возвращает настраиваемую строку.
   // fileName_ - имя файла.
   // bVariables_ - Записать переменные.
   // bPredicates_ - Записать предикаты и их таблицы истинности.
   // bIntegrityLimitation_ - Записать исходное ограничение целостности.
   // bGenerations_ - Записать поколения.
   // bFitness - Выводить значение фитнес функции.
   // bTrueCondition_ - Истинность каждого условия.
   // bUsefulnessCondition_ - Полезность условия.
   // countIndividuals_ - Количество первых особей.
   QString StringCustom(bool bVariables_ = true, bool bPredicates_ = true, bool bIntegrityLimitation_ = true, bool bGeneration_ = true, bool bFitness_ = true, bool bTrueCondition_ = false, bool bUsefulnessCondition_ = false, size_t countIndividuals_ = SIZE_MAX) const;

   // Запись в файл.
   // fileName_ - имя файла.
   // bVariables_ - Записать переменные.
   // bPredicates_ - Записать предикаты и их таблицы истинности.
   // bIntegrityLimitation_ - Записать исходное ограничение целостности.
   // bGenerations_ - Записать поколения.
   // bFitness - Выводить значение фитнес функции.
   // bTrueCondition_ - Истинность каждого условия.
   // bUsefulnessCondition_ - Полезность условия.
   // countIndividuals_ - Количество первых особей.
   // !> emit signal error.
   void WriteInFile(const QString& fileName_, bool bVariables_ = true, bool bPredicates_ = true, bool bIntegrityLimitation_ = true, bool bGeneration_ = true, bool bFitness_ = true, bool bTrueCondition_ = false, bool bUsefulnessCondition_ = false, size_t countIndividuals_ = SIZE_MAX) const;

   // Запустить алгоритм с заданием параметров.
   // countIndividuals_ - количество особей
   // countIterations_ - количество итераций
   // percentMutationArguments_ - процент мутаций аргументов в предикате в одном условии
   // countSkipMutationArg_ - количество последних итераций которых не затронет мутация аргументов
   // percentMutationPredicates_ - процент мутаций предикатов (полностью, с аргументами) в одном условии
   // countSkipMutationPred_ - количество последних итераций которых не затронет мутация предикатов
   // percentIndividualsUndergoingMutation_ - процент особей которые будут подвергнуты мутациям (в каждом поколении)
   // bContinue_ - продождить расчет (не создавать первое поколение рандомно, а использовать уже имеющееся).
   void Start(int countIndividuals_, int countIterations_, double percentMutationArguments_, int countSkipMutationArg_, double percentMutationPredicates_, int countSkipMutationPred_, double percentIndividualsUndergoingMutation_ = 100, bool bContinue_ = false, bool bBoost_ = false);

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
   void signalEnd(bool noError_) const;
   void signalError(const CException& error_) const;


private:
   //  ========================= З а к р ы т ы е   м е т о д ы ==========================

   // Считывает и заносит условия из строки.
   void SetConditionsFromString(const QString& str_);

   // Записывает условие в строку.
   QString StringCondition(const SCondition& condition_) const;

   // Записывает ограничение целостности в строку.
   QString StringIntegrityLimitation(const TIntegrityLimitation& integrityLimitation_, bool bInsertNewLine_ = false, bool bTrueCondition_ = false, bool bUsefulnessCondition_ = false) const;

   // Заполняет вектор аргументов (шаблонных) случайныими значениями. countArgs_ - размер вектора, maxArg - число самого большого присутствующего аргумента.
   void FillRandomArguments(std::vector<int>& vArguments_, int& maxArg_) const;

   // Сгенерировать первое поколение рандомно.
   void CreateFirstGenerationRandom(size_t count_);

   // Скрещивание только по предикатам.
   TIntegrityLimitation CrossingOnlyPredicates(const TIntegrityLimitation& parent1_, const TIntegrityLimitation& parent2_) const;

   // Селекция. Выбираются лучшие (по фитнесс функции) CountSurvivors_ особей из individuals_, т.е. полная замена, родителей "убиваем".
   void Selection(TGeneration&& individuals_, size_t countSurvivors_);

   // Мутация аргументов в предикате.
   void MutationArguments(TIntegrityLimitation& individual_, double ratio_) const;

   // Мутация предикатов.
   void MutationPredicates(TIntegrityLimitation& individual_, double ratio_) const;

   // Возвращает количество всех предикатов во всем ограничении.
   size_t CountAllPredicates(const TIntegrityLimitation& individual_) const;

   // Возвращает корректность условия (обе части должны быть не пусты).
   bool IsCorrectCondition(const SCondition& cond_) const;

   // Возвращает истинность условия.
   // Первый возвращаемый параметр - истинность условия.
   // Второй возвращаемый параметр - полезность условия: false - не полезное --
   // когда в условии левая часть всегда ложна или содержатся одинаковые предикаты
   // в обоих частях, true - полезное.
   std::pair<bool, bool> IsTrueCondition(const SCondition& cond_) const;
   std::pair<bool, bool> IsTrueConditionOld(const SCondition& cond_) const;

   // Возвращает true - если в условии есть предикат в котором все аргументы равны -1 (т.е. любые '~'),
   // при этом мапка будет не полной, так как поиск прекращается, так как это условие уже бессмыслено.
   // mapPredicates_ мапка со всеми предикатами у которых хотябы один аргумент равен -1.
   // Мапка содержит предикат и его таблицу истинности без учета аргументов которые равны -1.
   // Например если предикат содержит 2 аргумента и один из них это ~,
   // то таблица будет хранить условия истинности для набора из 1 аргумента.
   bool getPredicatesWithAnyArgument(const SCondition& Cond_, std::map<SPredicateTemplate, std::vector<bool>>& mapPredicates_) const;

   // Возвращает true, если таблицы и аргументы заполнены успешно (зависит от параметра bSkipPredIfFalse_).
   // partCond_ - часть условия для которого формируется таблица истинности и вектор шаблонных предикат без -1.
   // truthTable_ - таблица истинности предикатов из части условия partCond_.
   // vTemplArgsPart_ - вектор шаблонных аргументов предикат для которых сформирована таблица (без -1).
   // mapArgsForPred_ - набор переменных, при которых предикат с индексовм key может быть истинен.
   // vSubsRealArgsPart_ - вектор переменных для шаблонов.
   // bSkipPredIfFalse_ - нужно ли пропускать предикат, если он всегда ложен. Если нет, тогда возвращает false,
   // при хотябы одном ложном предикате, иначе вернет false только когда все предикаты ложны.
   bool getPredicateWithTableAndPredivateWithArg(const TPartCondition& partCond_, std::vector<CSparseTruthTable>& truthTable_,
      std::vector<std::vector<size_t>>& vTemplArgsPart_, const std::map<size_t, std::vector<std::set<size_t>>>& mapArgsForPred_,
      const std::vector<std::set<size_t>>& vSubsRealArgsPart_, bool bSkipPredIfFalse_) const;

   // Заполняет таблицу истинности и вектор шаблонных аргументов для данной таблицы
   // для шаблонного предиката удаляя от туда все шаблоны равные -1.
   // Возвращает true, если таблица успешно заполнена, false - если предикат всегда ложен.
   // predTemp_ - шаблонный предикат, для которого вычисляется таблица и аргументы.
   // truthTable_ - заполненная таблица истинности.
   // vTemplArgs_ - заполненный новый вектор шаблонных аргументов (исключая -1).
   // vWildcardArgsForTemp_ - общий вектор подстановочных аргументов.
   // vRealArgsForPred_ - значения аргументов, при которых предикат predTemp_ может быть истеннен.
   // Используется в случае, если у предиката какой то шаблонный аргумент равен -1.
   bool getTruthTableAndArgumentsForPredicate(const SPredicateTemplate& predTemp_, CSparseTruthTable& truthTable_,
      std::vector<size_t>& vTemplArgs_, const std::vector<std::set<size_t>>& vWildcardArgsForTemp_,
      const std::vector<std::set<size_t>>& vRealArgsForPred_) const;

   // Возвращает индекс родителя из поколения. Турнирная функция выбора.
   size_t SelectRandParent() const;

   // Возвращает индексы двух разных родителей из поколения.
   // Использует турнирный отбор.
   std::pair<size_t, size_t> GetPairParents() const;

   // Сортирует поколение в порядке убывания фитнес функции.
   void SortGenerationDescendingOrder(TGeneration& generation_) const;

   // Фитнес функция.
   // Возвращает значение приспособленности (фитнеса) для ограничения целостности.
   // Считается отдельно для каждого условия FC - фитнес одного условия.
   // Конечное значение это сумма всех FC.
   // Расчет для условия:
   // Считается кол-во предикатов (N) в одном условии целостности.
   // Значение для предиката равно P / N, где P = [0, 1] - цена предиката.
   // Цена добавления предиката устанавливается пользователем P = (m_costAddingPredicate).
   // Цена удаления предиката равна P = 0.
   // Цена предиката который есть в оригинальном условии вычисляется так:
   // Все аргументы совпали - P = 1.
   // Все аргументы изменились - P = нижняя_граница (устанавливается пользователем).
   // dif аргуметнов поменялось из tot то P = нижняя_граница + ((tot-dif)/tot) * (1 - нижняя_граница).
   double FitnessFunction(TIntegrityLimitation& conds_) const;

   // ----------------------- Вспомогательные функции для фитнеса -----------------------

   // Возвращает индексы предикатов из условия, которые совпадают с sample_, и количество отличий в порядке возрастания отличий в аргументах.
   // Возвращает multimap<число_отличий, индекс_предиката>.
   std::multimap<int, size_t> findMinDifference(const SPredicateTemplate& sample_, const TPartCondition& verifiable_) const;

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

   // Возвращает true, если есть полностью одинаковые предикаты в обоих частях условия.
   bool areThereSamePredicatesDifferentParts(const SCondition& cond_) const;

   // Возвращает количество шаблонных аргументов, которые используются в единственном экземплляре.
   size_t countSingleArguments(const SCondition& cond_) const;

   // --------------------------- Функции работы со строками ----------------------------

   static QString highlightBlock(const QString& str_, qsizetype& index_);
   static QString highlightName(const QString& str_, qsizetype& index_);
   static bool hasSymbol(const QString str_, qsizetype& index_, QChar symbol_);
};