#pragma once
#include <vector>

template<class T>
class CCounter
{
   std::vector<T> m_vCounter;
   const T m_upperBound;
   const T m_lowerBound;

public:

   CCounter(const T& lowerBound_, const T& upperBound_, const std::vector<T>& counter_) : m_upperBound(upperBound_), m_lowerBound(lowerBound_), m_vCounter(counter_) {}
   ~CCounter() = default;

   CCounter& operator++()
   {
      const T upperBound = m_upperBound - 1;

      for (size_t i = m_vCounter.size() - 1; i != SIZE_MAX; --i)
      {
         if (m_vCounter.at(i) < upperBound)
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
      const T upperBound = m_upperBound - 1;

      for (size_t i = m_vCounter.size() - 1; i != SIZE_MAX; --i)
      {
         if (m_vCounter.at(i) > m_lowerBound)
         {
            --m_vCounter[i];
            break;
         }
         else
         {
            m_vCounter[i] = upperBound;
         }
      }

      return *this;
   }

   const std::vector<T>& data() const
   {
      return m_vCounter;
   }
};