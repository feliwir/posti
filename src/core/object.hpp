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
}