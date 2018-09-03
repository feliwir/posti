#pragma once

namespace ps
{
    enum class ObjectFlag
    {
        Literal,
        Executable
    };

    enum class ObjectAccess
    {
        Unlimited,
        ReadOnly,
        ExecuteOnly,
        None,
    };

    class Object
    {
        public:
        inline bool IsExecutable() {
            return m_flag == ObjectFlag::Executable;
        }

        inline ObjectAccess GetAccess() {
            return m_access;
        }

        protected:
        ObjectFlag m_flag = ObjectFlag::Literal;
        ObjectAccess m_access = ObjectAccess::Unlimited;
    };

    class NameObject final : public Object
    {
    public:
      inline NameObject(std::string_view view,bool executable = true)
      {
        m_name = view;
        m_flag = executable ? ObjectFlag::Executable : ObjectFlag::Literal;
      }
    private:
      std::string m_name;
    };

    class IntegerObject final : public Object
    {
    public:
      inline IntegerObject(const int value)
      {
        m_value = value;
      }
    private:
      int m_value;
    };

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