#pragma once
#include "../object.hpp"
#include <functional>
#include <memory>
#include <stack>

namespace ps
{
    class OperandObject final : public Object
    {
    public:
      inline OperandObject(std::function<void()> func)
      {
        m_func = func;
        m_type = ObjectType::Operand;
      }

      inline void Execute()
      {
        m_func();
      }

    private:
      std::function<void()> m_func;
    };
}