#include "interpreter.hpp"
#include "parser.hpp"
#include "builtins.hpp"
#include "objects/name.hpp"
#include <iostream>
#include <string>

ps::Interpreter::Interpreter(ScriptMode mode)
{
  m_mode = mode;
  m_systemDict = Builtins::CreateDictionary(this);

  m_dictStack.push_back(m_systemDict);
}

std::shared_ptr<ps::Object> ps::Interpreter::DictLookup(std::shared_ptr<Object> name)
{
  auto str = std::static_pointer_cast<NameObject>(name)->GetName();

  for (auto rit = m_dictStack.rbegin(); rit != m_dictStack.rend(); ++rit)
  {
    auto key = rit->find(str);
    if (key != rit->end())
    {
      return key->second;
    }
  }

  std::cerr << "Missing name: " << str << std::endl;

  return nullptr;
}

void ps::Interpreter::Load(std::istream &input)
{
  Parser parser(input);

  while (auto obj = parser.GetObject())
  {
    //Push literal objects to the operand stack
    if (!obj->IsExecutable())
      m_opStack.push(obj);
    else
    {
      if (obj->GetType() == ObjectType::Name)
      {
        //lookup in the dictionary
        auto value = DictLookup(obj);
      }
    }
  }
}
