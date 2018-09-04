#pragma once
#include "../object.hpp"

namespace ps
{
    class OperandObject final : public Object
    {
    public:
      inline OperandObject(const int value)
      {
        m_value = value;
      }
    private:
      int m_value;
    };
}