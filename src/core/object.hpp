#pragma once
#include <memory>

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

class Object : public std::enable_shared_from_this<Object>
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

    template<class T>
    inline std::shared_ptr<T> Cast()
    {
      return std::static_pointer_cast<T>(shared_from_this());
    }

  protected:
    ObjectFlag m_flag = ObjectFlag::Literal;
    ObjectAccess m_access = ObjectAccess::Unlimited;
    ObjectType m_type = ObjectType::None;
};
} // namespace ps