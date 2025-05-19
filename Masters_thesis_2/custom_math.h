#pragma once
#include <cmath>

inline size_t pow(size_t base_, size_t exp_)
{
   size_t result = 1;
   while (exp_ > 0)
   {
      if (exp_ % 2 == 1)
         result *= base_;

      base_ *= base_;
      exp_ /= 2;
   }

   return result;
}

inline size_t intLog(size_t base_, size_t x_)
{
   return static_cast<size_t>(log(x_) / log(base_) + 0.01);
}