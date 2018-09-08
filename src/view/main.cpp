#include <fstream>
#include <cxxopts.hpp>
#include "interpreter.hpp"

int main(int argc, char **argv)
{
  cxxopts::Options options("psview", "A viewer for postscript files/programs.");

  std::string fileInput;

  options.add_options()("f,file", "File name", cxxopts::value<std::string>(fileInput));

  auto result = options.parse(argc, argv);

  if (fileInput.empty())
  {
    std::cout << "Please provide atleast one input file!";
    options.help();
    return -1;
  }

  std::ifstream fin(fileInput);

  if (fin.fail())
  {
    std::cout << "Failed to open the specified file!";
    options.help();
    return -1;
  }

  ps::Interpreter psi;

  psi.Load(fin);

  return 0;
}