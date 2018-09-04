add_library(pscore STATIC 
interpreter.cpp interpreter.hpp
object.hpp
objects/integer.hpp
objects/name.hpp
objects/operand.hpp
objects/real.hpp
objects/string.hpp
graphicsstate.hpp
parser.cpp parser.hpp
util.hpp)

target_link_libraries(pscore b2d)