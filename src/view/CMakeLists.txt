add_executable(psview main.cpp)

target_link_libraries(psview pscore)
target_include_directories(psview PRIVATE ${POSTI_ROOT}/deps/cxxopts/include)