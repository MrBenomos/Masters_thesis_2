#pragma once
#include <vector>
#include <bitset>
#include <stdexcept>

// Проверка на переполнение умножения.
inline bool WillMultiplyOverflow(size_t a, size_t b)
{
   if (a == 0 || b == 0)
      return false;

   return a > SIZE_MAX / b;
}

// Количество размещений.
inline size_t NumberOfPlacements(size_t total_, size_t place_)
{
   size_t result = 1;
   for (; place_ != 0; --place_, --total_)
   {
      if (WillMultiplyOverflow(result, total_))
         throw std::out_of_range("It is not possible to calculate the number of placements for the specified numbers");

      result *= total_;
   }

   return result;
}

// Счетчик.
// m_lowerBound - нижняя граница (включительно).
// m_upperBound - верхняя граница (не включительно).
template<class T>
class CCounter
{
   const T m_lowerBound;
   const T m_upperBound;
   std::vector<T> m_vCounter;

   void throwError() const
   {
      if (m_lowerBound >= m_upperBound)
         throw std::invalid_argument("The lower bound is greater than or equal to the upper bound");

      if (m_vCounter.empty())
         throw std::invalid_argument("Attempt to create an empty counter");
   }

public:

   CCounter(const T& lowerBound_, const T& upperBound_, const std::vector<T>& counter_) :
      m_lowerBound(lowerBound_), m_upperBound(upperBound_), m_vCounter(counter_)
   {
      throwError();
      for (const T& v : m_vCounter)
         if (v < m_lowerBound || v >= m_upperBound)
            throw std::invalid_argument("The vector contains an out-of-bounds element");
   }

   CCounter(const T& lowerBound_, const T& upperBound_, size_t countElements_, const T& startValue_ = m_lowerBound) :
      m_lowerBound(lowerBound_), m_upperBound(upperBound_), m_vCounter(countElements_, startValue_)
   {
      throwError();

      if (startValue_ < m_lowerBound || startValue_ >= m_upperBound)
         throw std::invalid_argument("The starting value is not in the specified range");
   }
   ~CCounter() = default;

   CCounter& operator++()
   {
      const T maximum = m_upperBound - T(1);

      for (size_t i = m_vCounter.size() - 1; i != SIZE_MAX; --i)
      {
         if (m_vCounter.at(i) < maximum)
         {
            ++m_vCounter[i];
            break;
         }
         else
         {
            m_vCounter[i] = m_lowerBound;
         }
      }

      return *this;
   }

   CCounter& operator--()
   {
      const T maximum = m_upperBound - T(1);

      for (size_t i = m_vCounter.size() - 1; i != SIZE_MAX; --i)
      {
         if (m_vCounter.at(i) > m_lowerBound)
         {
            --m_vCounter[i];
            break;
         }
         else
         {
            m_vCounter[i] = maximum;
         }
      }

      return *this;
   }

   const std::vector<T>& get() const
   {
      return m_vCounter;
   }

   // Возвращает количество итераций необходимых от минимума до максимума.
   size_t countIterations() const
   {
      if (m_vCounter.empty())
         return 0;

      const size_t countVariants = static_cast<size_t>(m_upperBound - m_lowerBound);
      size_t count = countVariants;
      for (size_t i = 1; i < m_vCounter.size(); ++i)
      {
         if (WillMultiplyOverflow(count, countVariants))
            throw std::out_of_range("The number of iterations exceeds the allowed range");

         count *= countVariants;
      }

      return count;
   }
};

// Счетчик без повторяющихся элементов.
// m_lowerBound - нижняя граница (включительно).
// m_upperBound - верхняя граница (не включительно).
template<class T>
class CCounterWithoutRepeat
{
   enum EEnds : size_t
   {
      eMaximum,
      eMinimum
   };

   const T m_lowerBound;
   const T m_upperBound;

   std::vector<bool> m_vUsed;
   size_t m_maxSize = 0;
   size_t m_minSize = 0;

   std::vector<T> m_vCounter;

   void throwError() const
   {
      if (m_lowerBound >= m_upperBound)
         throw std::invalid_argument("The lower bound is greater than or equal to the upper bound");

      if (m_vCounter.empty())
         throw std::invalid_argument("Attempt to create an empty counter");
   }

   bool contains(const T& value_) const
   {
      return m_vUsed.at(static_cast<size_t>(value_ - m_lowerBound));
   }

   void add(const T& value_)
   {
      m_vUsed[static_cast<size_t>(value_ - m_lowerBound)] = true;
   }

   void remove(const T& value_)
   {
      m_vUsed[static_cast<size_t>(value_ - m_lowerBound)] = false;
   }

   void rebuildMask()
   {
      std::fill(m_vUsed.begin(), m_vUsed.end(), false);

      for (const T& v : m_vCounter)
         add(v);
   }

   void recalculateSizes()
   {
      if (m_vCounter[m_minSize] == T(m_lowerBound - T(m_minSize)))

         if (m_vCounter[m_maxSize] == T(m_upperBound - T(m_maxSize + 1)))
            ++m_maxSize;
   }

public:

   // counter_ - начальный вектор.
   CCounterWithoutRepeat(const T& lowerBound_, const T& upperBound_, const std::vector<T>& counter_) :
      m_lowerBound(lowerBound_), m_upperBound(upperBound_), m_vCounter(counter_)
   {
      throwError();

      m_vUsed.resize(m_upperBound - m_lowerBound, false);

      for (const T& v : m_vCounter)
      {
         if (contains(v))
            throw std::invalid_argument("The vector contains the same elements");

         if (v < m_lowerBound || v >= m_upperBound)
            throw std::invalid_argument("The vector contains an out-of-bounds element");

         add(v);
      }

      if (m_vCounter == minimum())
         m_minSize = m_vCounter.size();
      else if (m_vCounter == maximum())
         m_maxSize = m_vCounter.size();
   }

   // countElements_ - количество элементов в векторе.
   // minimal_ = true - заполнить минимальным значением, иначе максимальным.
   CCounterWithoutRepeat(const T& lowerBound_, const T& upperBound_, size_t countElements_, bool minimal_ = true) :
      m_lowerBound(lowerBound_), m_upperBound(upperBound_), m_vCounter(countElements_)
   {
      throwError();

      m_vUsed.resize(m_upperBound - m_lowerBound, false);

      if (countElements_ > m_upperBound - m_lowerBound)
         throw std::invalid_argument("The size of the vector exceeds the specified range");

      if (minimal_)
      {
         for (size_t i = 0; i < countElements_; ++i)
         {
            const T value = T(m_lowerBound + i);
            m_vCounter[i] = value;
            add(value);
         }

         m_minSize = m_vCounter.size();
      }
      else
      {
         for (size_t i = 0; i < countElements_; ++i)
         {
            const T value = T(m_upperBound - T(i + 1));
            m_vCounter[i] = value;
            add(value);
         }

         m_maxSize = m_vCounter.size();
      }
   }

   ~CCounterWithoutRepeat() = default;

   CCounterWithoutRepeat& operator++()
   {
      bool bSuccess = false;
      for (size_t i = m_vCounter.size() - 1; i != SIZE_MAX; --i)
      {
         remove(m_vCounter[i]);
         for (T newValue = m_vCounter[i] + T(1); newValue < m_upperBound; ++newValue)
         {
            if (!contains(newValue))
            {
               add(newValue);
               m_vCounter[i] = newValue;
               bSuccess = true;
               break;
            }
         }

         if (bSuccess)
         {
            T minimum = m_lowerBound;
            for (size_t j = i + 1; j < m_vCounter.size(); ++j)
            {
               for (T newValue = minimum; newValue < m_upperBound; ++newValue)
               {
                  if (!contains(newValue))
                  {
                     add(newValue);
                     m_vCounter[j] = newValue;
                     minimum = ++newValue;
                     break;
                  }
               }
            }

            // Проверяем является ли последний из минимальных элементов минимальным.
            if (m_minSize != 0 && m_vCounter[m_minSize - 1] == T(m_lowerBound + T(m_upperBound + m_minSize)))
               --m_minSize;

            // Проверяем не является ли следующий за последним из максимальных элементов максимальным.
            if (m_vCounter[m_maxSize] == T(m_upperBound - T(m_maxSize + 1)))
               ++m_maxSize;

            break;
         }
      }

      if (!bSuccess)
         rebuildMask();

      return *this;
   }

   CCounterWithoutRepeat& operator--()
   {
      bool bSuccess = false;
      for (size_t i = m_vCounter.size() - 1; i != SIZE_MAX; --i)
      {
         remove(m_vCounter[i]);
         for (T newValue = m_vCounter[i] - T(1); newValue != m_lowerBound - T(1); --newValue)
         {
            if (!contains(newValue))
            {
               add(newValue);
               m_vCounter[i] = newValue;
               bSuccess = true;
               break;
            }
         }

         if (bSuccess)
         {
            T maximum = m_upperBound - T(1);
            for (size_t j = i + 1; j < m_vCounter.size(); ++j)
            {
               for (T newValue = maximum; newValue != m_lowerBound - T(1); --newValue)
               {
                  if (!contains(newValue))
                  {
                     add(newValue);
                     m_vCounter[j] = newValue;
                     maximum = --newValue;
                     break;
                  }
               }
            }

            // Проверяем является ли последний из максимальных элементов максимальным.
            if (m_maxSize != 0 && m_vCounter[m_maxSize - 1] == T(m_upperBound - T(m_maxSize + 1)))
               --m_maxSize;

            // Проверяем не является ли следующий за последним из минимальных элементов минимальным.
            if (m_vCounter[m_minSize] == T(m_lowerBound + T(m_minSize)))
               ++m_minSize;

            break;
         }
      }

      if (!bSuccess)
         rebuildMask();

      return *this;
   }

   const std::vector<T>& get() const
   {
      return m_vCounter;
   }

   // Возвращает максимально возможный вектор.
   std::vector<T> maximum() const
   {
      std::vector<T> result(m_vCounter.size());
      T value = m_upperBound;
      for (T& v : result)
         v = --value;

      return result;
   }

   // Возвращает минимально возможный вектор.
   std::vector<T> minimum() const
   {
      std::vector<T> result(m_vCounter.size());
      T value = m_lowerBound - T(1);
      for (T& v : result)
         v = ++value;

      return result;
   }

   bool isMaximum() const
   {
      return m_maxSize == m_vCounter.size();
   }

   bool isMinimum() const
   {
      return m_minSize == m_vCounter.size();
   }

   // Возвращает максимальное количество итераций.
   size_t countIterations() const
   {
      return NumberOfPlacements(static_cast<size_t>(m_upperBound - m_lowerBound), m_vCounter.size());
   }
};