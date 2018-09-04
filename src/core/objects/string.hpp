#pragma once
#include "../object.hpp"
#include <string_view>
#include <string>

namespace ps
{
    class StringObject final : public Object
    {
    public:
      inline StringObject(std::string_view value)
      {
        m_value = value;
        m_type = ObjectType::String;
      }

      inline std::string& GetValue()
      {
        return m_value;
      }
    private:
      std::string m_value;
    };
}