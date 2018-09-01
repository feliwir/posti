add_library(pscore STATIC 
interpreter.cpp interpreter.hpp
object.hpp
parser.cpp parser.hpp
util.hpp)

target_link_libraries(pscore b2d)