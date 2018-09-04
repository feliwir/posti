#pragma once
#include "../object.hpp"

namespace ps
{
    class IntegerObject final : public Object
    {
    public:
      inline IntegerObject(const int value)
      {
        m_value = value;
      }

      inline int GetValue()
      {
        return m_value;
      }
    private:
      int m_value;
    };
}