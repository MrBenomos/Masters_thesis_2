#pragma once
#include <vector>
#include <map>
#include <set>

#include <QString>

// Предикат.
// Хранит имя предиката name и его таблицу истинности table.
// Таблица записывается по порядку переменных.
// 
// Пример:
// Есть двуместный предикат P. Известно, что у него таблица истинности для 3 возможных переменных.
// Считаем что переменные названы цифрами от 0 до N, где N = <кол-во_переменных> - 1. (в примере N = 2).
// То есть у нас переменные: 0, 1, 2. Таблица должна хранить значения для набора переменных, а так как у нас
// предикат двуместный то каждому значению в таблице будет сопоставлено 2 аргумента. (0,0 = x; 0,1 = y; ...).
// Так как у нас всего 3 возможных переменных, для которых определена таблица, размер таблицы получается 3^2=9.
// Таблица должна храниться по порядку, т.е. 0,0; 0,1; 0,2; 1,0; 1,1; 1,2; 2,0; 2,1; 2,2.

// Возвращает индекс для набора переменных (точнее их индексов).
// Нумерация переменных начинается с 0.
size_t GetIndex(size_t countVariables_, const std::vector<size_t>& args_);

struct SPredicate
{
   QString name;            // имя предиката
   std::vector<bool> table; // таблица истинности

   // Возвращает индекс таблицы истинности для набора переменных (точнее их индексов).
   // Нумерация переменных начинается с 0.
   // При ошибке возвращает SIZE_MAX.
   size_t GetIndex(size_t countVariables_, const std::vector<size_t>& args_) const;

   // Возвращает набор переменных (их индексы) для индекса таблицы истинности.
   // Нумерация переменных начинается с 0.
   // При ошибке возвращает пустой вектор.
   const std::vector<size_t> GetArgs(size_t countVariables_, size_t indexArguments_) const;
};

// Хранилище предикат с переменными.
// Класс хранит набор предикат и переменных, для удобной работы с предикатами.
// Разные предикаты могут иметь разное количество аргументов, т.е. может быть P(x) и Q(x, y).
// У всех предикатов ровно N * M значений, где N - количество всех переменных, M - количество
// принимаемых переменных (аргументов), т.о. у Q(x,y) значение M = 2.
// 
// Переменные не могут меняться после задания хотя бы одного предиката, т.к.
// его таблица истинности будет уже не действительна.
class CPredicatesStorage
{
   std::map<QString, size_t> m_mapVariables;
   std::vector<QString> m_vVariables;

   std::map<QString, size_t> m_mapPredicates;
   std::vector<SPredicate> m_vPredicates;

   mutable std::vector<std::vector<std::set<size_t> > > m_vHashesUsedVar;

public:

   // ================ Функции считывания / добавления данных ==================

   // Задает переменные.
   // !> exception если существуют предикаты.
   void SetVariables(const std::set<QString>& variables_);

   // Задает переменные. Возвращает true если все переменные успешно добавлены,
   // false - если были дубликаты переменных.
   // !> exception если существуют предикаты.
   bool SetVariables(const std::vector<QString>& variables_);

   // Задает переменные.
   // Форматирование: Символы до пробела или ',' считаются одним именем переменной.
   // Пробелы игнорируются, в имени не может быть зарезервированных символов.
   // !> exception если существуют предикаты.
   // !> exception если некорректное имя переменной.
   // !> exception если нет переменных в строке str_.
   void SetVariables(const QString& str_);

   // Добавляет предикаты.
   // Форматирование: Первые символы до '(' или пробельного символа - имя предиката.
   // После имени (в скобках, а можно и без них) число принемаемых аргументов.
   // Затем идет таблица истинности: Через пробел/запятую идут переменные которые истинны для этого предиката.
   // !> exception при некорректном имени предиката или переменной.
   // !> exception если поймана попытка добавить предикат с уже существующим именем.
   // !> exception если нет количества аргументов или недостаточное кол-во аргументов в строке таблицы.
   void AddPredicates(const QString& str_);

   // ========================= Вывод данных в строку =========================

   // Возвращает строку с переменными.
   QString StringVariables() const;

   // Возвращает строку с предикатами и их таблицами истинности.
   QString StringPredicatesWithTable() const;

   // Возвращает имя предиката с аргументами по индксам.
   // indexPredicate_ - индекс предиката, indexArguments_ - индекс таблицы истинности.
   // !> exception если нет предиката с индексом indexPredicate_.
   // !> exception если нет индекса для таблицы истинности indexArguments_.
   QString StringPredicateWithArg(size_t indexPredicate_, size_t indexArguments_) const;

   // =========================== Получение данных ============================

   // Возвращает вектор переменных.
   const std::vector<QString>& GetVariables() const;

   // Возвращает вектор индексов использующихся переменных в таблице истинности в предикате.
   const std::vector<std::set<size_t>>& GetVariablesUsedInPredicate(size_t indexPredicate_) const;

   // Получить индекс предиката по его названию.
   // Если такого предиката нет - вернет SIZE_MAX.
   size_t GetIndexPredicate(const QString& namePredicate_) const;

   // Получить индекс таблицы истинности по переменным arguments_ для предиката с индексом indexPredicate_.
   // Если такого предиката или таких переменных нет - вернет SIZE_MAX.
   size_t GetIndexArgument(size_t indexPredicate_, const std::vector<QString>& arguments_) const;

   // Возвращает предикат с индексом indexPredicate_.
   // !> exception если индекс невалиден.
   const SPredicate& GetPredicate(size_t indexPredicate_) const;

   // Получить значение предиката, при заданных переменных.
   // indexPredicate_ - индекс предиката, indexArguments_ - индекс таблицы истинности.
   // !> exception если нет предиката с индексом indexPredicate_.
   // !> exception если нет индекса для таблицы истинности indexArguments_.
   bool GetValuePredicate(size_t indexPredicate_, size_t indexArguments_) const;

   // Получить имя предиката по его индексу
   // !> exception если индекс невалиден.
   QString GetPredicateName(size_t indexPredicate_) const;

   // Получить набор переменных по индексу предиката и аргумента.
   // !> exception если нет предиката с индексом indexPredicate_.
   // !> exception если нет индекса для таблицы истинности indexArguments_.
   std::vector<QString> GetArgumentVariables(size_t indexPredicate, size_t indexArguments_) const;

   // ======================== Количественные функции =========================

   // Возвращает true, если нет ни одного предиката.
   bool IsEmpty() const;

   // Возвращает количество предикатов.
   size_t CountPredicates() const;

   // Возвращает количество переменных.
   size_t CountVariables() const;

   // Получить количество аргументов предиката с индексом indexPredicate_.
   // !> exception если индекс невалиден.
   size_t CountArguments(size_t indexPredicate_) const;

   // ======================== Вспомогательные функции ========================

   // Очищает переменныи и предикаты.
   void Clear();

   // Проверяет является ли символ зарезервированным.
   static bool isIllegalSymbol(QChar symb_);

private:

   // Выделяет имя.
   // Начинает с символа index_ и заканчивает пробельным символом или зарезервированным символом.
   // index_ будет указывать на следующий символ, после имени.
   static QString highlightName(const QString& str_, qsizetype& index_);
};

// Пропускает все пробельные символы, в строке, начиная с index_.
// index_ становится равным первому не пробельному символу или str_.size().
// Возвращает true, если был достигнут конец строки.
bool skipSpace(const QString& str_, qsizetype& index_);

// Пропускает все пробельные символы, в строке, начиная с index_.
// Затем пропускает, если есть символ skipSymbol_.
// index_ становится равным следующему символу после skipSymbol_ или первому не пробельному или str_.size().
void skipSpace(const QString& str_, qsizetype& index_, QChar skipSymbol_);