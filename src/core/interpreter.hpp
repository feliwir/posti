#pragma once
#include <istream>
#include <stack>

namespace ps
{
  class Object;

  class Interpreter
  {
  public:
    void Load(std::istream& input);

  private:
    static constexpr int MAX_BUFFER_SIZE = 4096;
    char m_buffer[MAX_BUFFER_SIZE];
    int m_bufSize;
    std::stack<std::shared_ptr<Object>> m_operands;
  };
}