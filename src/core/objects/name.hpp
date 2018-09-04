#pragma once
#include "../object.hpp"
#include <string_view>
#include <string>

namespace ps
{
    class NameObject final : public Object
    {
    public:
      inline NameObject(std::string_view view,bool executable = true)
      {
        m_name = view;
        m_flag = executable ? ObjectFlag::Executable : ObjectFlag::Literal;
        m_type = ObjectType::Name;
      }

      inline std::string& GetName()
      {
        return m_name;
      }
    private:
      std::string m_name;
    };
}