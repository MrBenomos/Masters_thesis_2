#pragma once
#include <QObject>
#include <QString>
#include <vector>
#include <map>

#include "random.h"

class QTextStream;

// Пропозициональная переменная (PropositionalVariable) 
struct SPropVar
{
   QString name;
   bool value;

   SPropVar(const QString& Name_, bool Value_) :
      name(Name_), value(Value_) {};

   bool operator<(const SPropVar& Var_) const
   { return name < Var_.name; }
};

enum EAddingError
{
   eSuccessfully,       // Успешное добавление переменной
   eUnknownVariable,    // Неизвестная переменная
   VariablePresent,     // Переменная уже присутствует
   OppositeCondition,   // Переменная используется в другой части условия
   eInvalidIndex        // Неверный индекс
};

class CGeneticAlgorithm : public QObject
{
   Q_OBJECT

   using TCond = std::vector<char>; // 0 - не использ., 1 - лево, 2 - право
   using TСondIntegrity = std::vector<TCond>;

public:
   CGeneticAlgorithm();
   ~CGeneticAlgorithm() = default;

   // Добавляет пропозициональную переменную, возвращает true если успешно добавлено, false иначе.
   bool AddVariable(const SPropVar& Variable_);
   // Добавляет условие, возвращает индекс этого условия.
   size_t AddCondition();
   // Добавляет переменную в левую часть условия.
   EAddingError AddVariableLeftSideCondition(size_t IndexCondition_, const QString& Name_);
   // Добавляет переменную в правую часть условия.
   EAddingError AddVariableRightSideCondition(size_t IndexCondition_, const QString& Name_);
   // Считывает и заносит переменные из строки, возвращает признак успешности прочтения строки.
   bool SetVariablesFromString(const QString& Str_, QString& StrError_);
   // Считывает и заносит условие из строки, возвращает признак успешности прочтения строки.
   bool SetConditionFromString(const QString& Str_, QString& StrError_);
   // Получает данные из файла, возвращает признак успешности заполнения данных.
   bool FillDataInFile(const QString& FileName_, QString& StrError_);
   // Записывает ограничение целостности (исходное) в файл.
   bool WriteСonditionIntegrityInFile(const QString& FileName_, QString& StrError_) const;
   // Записывает все текущие поколения в файл.
   bool WriteGenerationsInFile(const QString& FileName_, QString& StrError_) const;

   // Формирует строку с переменными, return true - успешно.
   bool GetVariables(QString& strVariables_, QString& strError_) const;
   // Формирует строку с условием целостности, return true - успешно.
   bool GetСondIntegrity(QString& strСondIntegrity_, QString& strError_) const;
   // Формирует строку со всеми поколениями, return true - успешно.
   bool GetAllGenerations(QString& strGenerations_, QString& strError_) const;
   // Формирует строку с переменными и условием целостности, return true - успешно.
   bool GetVarAndCond(QString& str_, QString& strError_) const;
   // Формирует строку с переменными и всеми поколениями, return true - успешно.
   bool GetVarAndGen(QString& str_, QString& strError_) const;

   void Start(unsigned int CountIndividuals_, size_t CountIterations_, bool UseMutation_ = false, unsigned int Percent_ = 0);

   void StartForThread(unsigned int CountIndividuals_, size_t CountIterations_, bool UseMutation, unsigned int Percent_);

   void Clear();

   bool HasGenerations() const;

   // Возвращает true, если символ является зарезервированным.
   static bool IllegalSymbol(QChar Symbol_);

signals:
   void signalProgressUpdate(int value_);
   void signalEnd();
   void signalError(const QString& strError_);


protected:
   void InitVectVar();
   static QString ErrorMessage(const QString& Message_ = "", const QString& Line_ = "", size_t Position_ = SIZE_MAX);

   // Записать в поток все пропозициональные переменные.
   bool WriteVarsInStream(QTextStream& Stream_, QString& StrError_) const;
   // Записать в поток ограничение целостности.
   bool WriteСondsInStream(QTextStream& Stream_, QString& StrError_, const TСondIntegrity& Conds_) const;


   void CreateFirstGenerationRandom(size_t Count_);
   // Скрещивание по генам (каждый ген берется у случайного родителя).
   TСondIntegrity CrossingByGenes(const TСondIntegrity& Parent1_, const TСondIntegrity& Parent2_) const;
   // Скрещивание по ограничениям целостности (каждое ограничение берется у случайного родителя).
   TСondIntegrity CrossingByConds(const TСondIntegrity& Parent1_, const TСondIntegrity& Parent2_) const;
   // Селекция. Выбираются лучшие (по фитнесс функции) CountSurvivors_ особей из Individuals_, т.е. полная замена, родителей "убиваем".
   void Selection(const std::vector<TСondIntegrity>& Individuals_, size_t CountSurvivors_);
   void Mutation(TСondIntegrity& Individuals_, double Ratio_) const;

   bool IsCorrectCondition(const TCond& Cond_) const;
   bool IsTrueCondition(const TCond& Cond_) const;
   double FitnessFunction(const TСondIntegrity& Conds_) const;
   void SortDescendingOrder();

private:
   std::map<SPropVar, size_t> m_mapVariables;
   std::vector<SPropVar> m_vVariables;

   TСondIntegrity m_vSpecified; // условие целостности (для финтес ф-ции)

   std::vector<TСondIntegrity> m_vGenerations;

   mutable CRandom m_rand;
};