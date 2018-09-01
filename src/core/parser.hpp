#pragma once
#include <istream>
#include <vector>
#include <memory>
#include "object.hpp"

namespace ps
{

  class Parser
  {
  public:
    Parser(std::istream& input);

    std::shared_ptr<Object> GetObject() const;

  private:
    std::istream& m_input;
  };
}