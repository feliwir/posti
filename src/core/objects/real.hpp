#pragma once
#include "../object.hpp"

namespace ps
{
    class RealObject final : public Object
    {
    public:
      inline RealObject(const float value)
      {
        m_value = value;
      }
    private:
      float m_value;
    };
}