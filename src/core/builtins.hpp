#pragma once
#include <map>
#include <memory>
#include <string>
#include "objects/operand.hpp"
#include "objects/integer.hpp"
#include "objects/real.hpp"

namespace ps
{
class Interpreter;
class Object;

class Builtins
{
  public:

    std::map<std::string, std::shared_ptr<Object>>& CreateDictionary(Interpreter *interpr);
  private:
    void CreateOperand(std::string_view name,std::function<void()> func);
    std::stack<std::shared_ptr<Object>> & GetStack();

    inline std::shared_ptr<Object> Top()
    {
      auto& s = GetStack();
      return s.top();
    }

    inline std::shared_ptr<Object> Pop()
    {
      auto& s = GetStack();
      auto result = s.top();
      s.pop();
      return result;
    }

    inline std::vector<std::shared_ptr<Object>> Pop(int n)
    {
      std::vector<std::shared_ptr<Object>> result;
      for(int i=0;i<n;++i)
        result.push_back(Pop());

      return result;
    }

    template<class T>
    inline T Pop()
    {
      return Cast<T>(Pop());
    }

    inline void Push(std::shared_ptr<Object> o)
    {
      auto& s = GetStack();
      s.push(o);
    }

    inline void Push(std::vector<std::shared_ptr<Object>>& objs)
    {
      for(auto o : objs)
        Push(o);
    }

    template<class T>
    inline void Push(T v);

    template<class T>
    inline T Cast(std::shared_ptr<Object>);

    template<typename F>
    inline void ArithmeticOp(F op)
    {
      auto a = Pop();
      auto b = Pop();

      if (a->GetType() == ObjectType::Integer && b->GetType() == ObjectType::Integer)
      {
        int ai = Cast<int>(a);
        int bi = Cast<int>(b);
        Push(op(ai,bi));
      }
      else
      {
        double av, bv;
        //TODO: implement double/real
      }
    }

    std::map<std::string, std::shared_ptr<Object>> m_dict;
    Interpreter* m_interpr;
};

template<>
inline void Builtins::Push<int>(int v)
{
  auto& s = GetStack();
  s.push(std::make_shared<IntegerObject>(v));
}

template<>
inline int Builtins::Cast<int>(std::shared_ptr<Object> o)
{
  return o->Cast<IntegerObject>()->GetValue();
}


} // namespace ps