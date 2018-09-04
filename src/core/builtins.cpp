#include "builtins.hpp"
#include "objects/operand.hpp"
#include "objects/integer.hpp"
#include "objects/real.hpp"
#include "interpreter.hpp"

std::map<std::string, std::shared_ptr<ps::Object>> ps::Builtins::CreateDictionary(Interpreter* interpr)
{
  std::map<std::string, std::shared_ptr<Object>> dict;
  auto& s = interpr->GetOperandStack();

  auto pop = [&s] () 
  { 
    auto result = s.top();
    s.pop();
    return result;
  };

  auto getInt = [&s](std::shared_ptr<ps::Object> o)
  {
    return std::dynamic_pointer_cast<IntegerObject>(o)->GetValue();
  };

  //STACK
  //POP
  dict["pop"] = std::make_shared<OperandObject>([&]() {
    s.pop();
  });

  //ARITHMETIC
  //ADD
  dict["add"] = std::make_shared<OperandObject>([&]() {
    auto a = pop();
    auto b = pop();

    if (a->GetType() == ObjectType::Integer && b->GetType() == ObjectType::Integer)
    {
      int sum = getInt(a) + getInt(b);

      s.push(std::make_shared<IntegerObject>(sum));
    }
  });


  return dict;
}
