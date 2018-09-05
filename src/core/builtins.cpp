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
    return std::static_pointer_cast<IntegerObject>(o)->GetValue();
  };

  //STACK
  //POP
  dict["pop"] = std::make_shared<OperandObject>([&]() {
    pop();
  });

  //STACK
  //EXCH
  dict["exch"] = std::make_shared<OperandObject>([&]() {
    auto first = pop();
    auto second = pop();
    s.push(first);
    s.push(second);
  });

  //STACK
  //DUP
  dict["dup"] = std::make_shared<OperandObject>([&]() {
    s.push(s.top());
  });

  //STACK
  //copy
  dict["copy"] = std::make_shared<OperandObject>([&]() {
    int n = getInt(pop());
    auto vec = std::vector<std::shared_ptr<ps::Object>>();
    for (int i = 0; i < n; i++)
    	vec.push_back(pop());
    for (int i = n - 1; i >= 0; i--)
    	s.push(vec[i]);
    for (int i = n - 1; i >= 0; i--)
    	s.push(vec[i]);
  });

  //STACK
  //index
  dict["index"] = std::make_shared<OperandObject>([&]() {
    int n = getInt(pop());
    auto vec = std::vector<std::shared_ptr<ps::Object>>();
    for (int i = 0; i < n; i++)
    	vec.push_back(pop());
    for (int i = n - 1; i >= 0; i--)
    	s.push(vec[i]);
    s.push(vec[n - 1]);
  });

  //STACK
  //roll
  dict["roll"] = std::make_shared<OperandObject>([&]() {
    int j = getInt(pop());
    int n = getInt(pop());
    // ??
  });

  //STACK
  //clear
  dict["clear"] = std::make_shared<OperandObject>([&]() {
    while(!s.empty())
    	s.pop();
  });

  //STACK
  //count
  dict["count"] = std::make_shared<OperandObject>([&]() {
    s.push(std::make_shared<IntegerObject>(s.size()));
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
