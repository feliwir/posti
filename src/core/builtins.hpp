#pragma once
#include <map>
#include <memory>
#include <string>

namespace ps
{
class Interpreter;
class Object;

class Builtins
{
  public:
    static std::map<std::string, std::shared_ptr<Object>> CreateDictionary(Interpreter *interpr);
};
} // namespace ps