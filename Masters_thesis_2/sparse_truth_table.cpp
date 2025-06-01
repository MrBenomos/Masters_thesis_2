#include "sparse_truth_table.h"

void CSparseTruthTable::addValue(const std::vector<size_t> value_)
{
	m_table.insert(value_);
}

void CSparseTruthTable::removeValue(const std::vector<size_t> value_)
{
	m_table.erase(value_);
}

void CSparseTruthTable::clear()
{
	m_table.clear();
}

bool CSparseTruthTable::hasValue(const std::vector<size_t> value_) const
{
	return m_table.contains(value_);
}
