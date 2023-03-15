CXX = g++
CXXFLAGS = -Wall -Wextra -Wno-unused-parameter -g -I../include/ --std=c++17
TESTERSRC = tester.cpp

ADDRSAN = -fsanitize=address -fno-sanitize-recover -fno-omit-frame-pointer -O0
UBSAN = -fsanitize=undefined -fsanitize=local-bounds -fsanitize=implicit-conversion -fno-sanitize-recover -fno-omit-frame-pointer -O0
SANCXX = clang++

main-test: FORCE 
	$(CXX) $(CXXFLAGS) $(TESTERSRC) test_main.cpp -o $@ 

addrsan-test: FORCE
	$(SANCXX) $(CXXFLAGS) $(ADDRSAN) $(TESTERSRC) test_main.cpp -o $@

ubsan-test: FORCE
	$(SANCXX) $(CXXFLAGS) $(UBSAN) $(TESTERSRC) test_main.cpp -o $@

coverage-test: FORCE
	$(CXX) $(CXXFLAGS) $(TESTERSRC) --coverage -fprofile-arcs -DCOVERAGE_TEST test_main.cpp -o $@ \
	-DLIGHTMODBUS_FULL \
	-DLIGHTMOBUS_DEBUG \
	-DLIGHTMODBUS_SLAVE_FULL \
	-DLIGHTMODBUS_MASTER_FULL \
	-x c ../include/lightmodbus/base.impl.h \
	-x c ../include/lightmodbus/debug.impl.h \
	-x c ../include/lightmodbus/master.impl.h \
	-x c ../include/lightmodbus/master_func.impl.h \
	-x c ../include/lightmodbus/slave.impl.h \
	-x c ../include/lightmodbus/slave_func.impl.h

FORCE: