#include "builtins.hpp"
#include "interpreter.hpp"

void ps::Builtins::CreateOperand(std::string_view name, std::function<void()> func)
{
  m_dict[std::string(name)] = std::make_shared<OperandObject>(func);
}

std::map<std::string, std::shared_ptr<ps::Object>> &ps::Builtins::CreateDictionary(Interpreter *interpr)
{
  //STACK
  //POP
  CreateOperand("pop", [this]() {
    Pop();
  });

  //EXCH
  CreateOperand("exch", [this]() {
    auto first = Pop();
    auto second = Pop();
    Push(first);
    Push(second);
  });

  //DUP
  CreateOperand("dup", [this]() {
    Push(Top());
  });

  //COPY
  CreateOperand("copy", [&]() {
    int n = Pop<int>();
    auto vec = Pop(n);
    std::reverse(vec.begin(), vec.end());
    Push(vec);
    Push(vec);
  });

  // //index
  // dict["index"] = std::make_shared<OperandObject>([&]() {
  //   int n = popInt();
  //   auto vec = std::vector<std::shared_ptr<ps::Object>>();
  //   for (int i = 0; i < n + 1; i++)
  //   	vec.push_back(pop());
  //   for (int i = n; i >= 0; i--)
  //   	s.push(vec[i]);
  //   s.push(vec[n]);
  // });

  // //roll
  // dict["roll"] = std::make_shared<OperandObject>([&]() {
  //   int j = popInt();
  //   int n = popInt();
  //   // ??
  // });

  //clear
  CreateOperand("clear", [this]() {
    auto s = GetStack();
    while(!s.empty())
      s.pop();
  });

  //count
  CreateOperand("count", [this]() {
    Push<int>(GetStack().size());
  });

  //ARITHMETIC
  //ADD
  CreateOperand("add", [this]() {
    ArithmeticOp(std::plus<>{});
  });

  //ADD
  CreateOperand("sub", [this]() {
    ArithmeticOp(std::minus<>{});
  });

  return m_dict;
}

std::stack<std::shared_ptr<ps::Object>> &ps::Builtins::GetStack()
{
  return m_interpr->GetOperandStack();
}