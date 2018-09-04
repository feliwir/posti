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
        m_type = ObjectType::Real;
      }
    private:
      float m_value;
    };
}