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
        m_type = ObjectType::String;
      }
    private:
      int m_value;
    };
}