#pragma once
#include <QRandomGenerator>

// Класс генерирующий рандомные числа в диапазоне [start; end]
class CRandom
{
public:

   // strart_ - число от которого будет начинаться генерация (включительно)
   // end_ - максимальное число при генерации (включительно)
   CRandom(quint64 start_ = 0, quint64 end_ = UINT64_MAX);

   quint64 Generate();
   // strart_ - число от которого будет начинаться генерация (включительно)
   // end_ - максимальное число при генерации (включительно)
   quint64 Generate(quint64 start_, quint64 end_);
   void SetBoundaries(quint64 start_, quint64 end_);

   // Позволяет при каждом новом запуске программы использовать новые числа
   void UseNewNumbers();
   void SetSeed(quint32 seed_);
   quint32 GetSeed() const;

private:

   void MakeCorrect();

   quint64 m_start;
   quint64 m_end;
   quint32 m_seed;
   QRandomGenerator m_generator;
};

// ======================================= Методы =======================================


inline CRandom::CRandom(quint64 start, quint64 end) : m_start(start), m_end(end)
{
   MakeCorrect();
}


inline quint64 CRandom::Generate()
{
   return m_generator.generate64() % (m_end - m_start + 1) + m_start;
}

inline quint64 CRandom::Generate(quint64 start_, quint64 end_)
{
   return m_generator.generate64() % (end_ - start_ + 1) + start_;
}


inline void CRandom::SetBoundaries(quint64 start_, quint64 end_)
{
   m_start = start_;
   m_end = end_;

   MakeCorrect();
}


inline void CRandom::SetSeed(quint32 seed_)
{
   m_seed = seed_;
   m_generator.seed(seed_);
}

inline quint32 CRandom::GetSeed() const
{
   return m_seed;
}


inline void CRandom::UseNewNumbers()
{
   SetSeed(QRandomGenerator::global()->generate());
}


inline void CRandom::MakeCorrect()
{
   if (m_start > m_end)
      std::swap(m_start, m_end);
}