#pragma once

namespace ps
{
enum class ObjectFlag
{
    Literal,
    Executable,
};

enum class ObjectAccess
{
    Unlimited,
    ReadOnly,
    ExecuteOnly,
    None,
};

enum class ObjectType
{
    None,
    Operand,
    Name,
    Real,
    Integer,
    String
};

class Object
{
  public:
    inline bool IsExecutable()
    {
        return m_flag == ObjectFlag::Executable;
    }

    inline ObjectAccess GetAccess()
    {
        return m_access;
    }

    inline ObjectType GetType()
    {
        return m_type;
    }

  protected:
    ObjectFlag m_flag = ObjectFlag::Literal;
    ObjectAccess m_access = ObjectAccess::Unlimited;
    ObjectType m_type = ObjectType::None;
};
} // namespace ps