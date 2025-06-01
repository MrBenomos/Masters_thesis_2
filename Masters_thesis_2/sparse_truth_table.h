#pragma once
#include <unordered_set>
#include <vector>
#include <functional>

struct VectorSizeTHash
{
   size_t operator()(const std::vector<size_t>& vec_) const
   {
      size_t seed = vec_.size(); // Учитываем размер вектора
      for (const auto& elem : vec_)
         seed ^= std::hash<size_t>{}(elem)+0x9e3779b9 + (seed << 6) + (seed >> 2);

      return seed;
   }
};

// Класс разряженная таблица истинности.
class CSparseTruthTable
{
   std::unordered_set<std::vector<size_t>, VectorSizeTHash> m_table;

public:

   CSparseTruthTable() = default;
   ~CSparseTruthTable() = default;

   void addValue(const std::vector<size_t> value_);
   void removeValue(const std::vector<size_t> value_);
   void clear();

   bool hasValue(const std::vector<size_t> value_) const;
};