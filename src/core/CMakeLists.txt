add_library(pscore STATIC 
processor.cpp processor.hpp
util.hpp)

target_link_libraries(pscore b2d)