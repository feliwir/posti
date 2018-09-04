#pragma once
#include "../object.hpp"

namespace ps
{
    class StringObject final : public Object
    {
    public:
      inline StringObject(const int value)
      {
        m_value = value;
      }
    private:
      int m_value;
    };
}