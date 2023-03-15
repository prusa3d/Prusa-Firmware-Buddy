#pragma once
#include <initializer_list>
#include <string>
#include <vector>
#include <functional>

#define LIGHTMODBUS_FULL
#include <lightmodbus/lightmodbus.h>

extern std::vector<uint16_t> regs;
extern std::vector<uint8_t> coils;
extern std::vector<uint8_t> request_data;
extern std::vector<uint8_t> response_data;

void build_request(const std::vector<int> &args);
void build_exception(uint8_t address, uint8_t function, ModbusExceptionCode code);
void parse_request();
void parse_response();
void dump_request();
void dump_response();
void dump_data();
void dump_queries();
void dump_master();
void dump_slave();
void dump_regs();
void dump_coils();
void assert_master_err(ModbusErrorInfo err);
void assert_master_ok();
void assert_slave_err(ModbusErrorInfo err);
void assert_slave_ok();
void assert_reg(int index, int value);
void assert_coil(int index, int value);
void assert_slave_ex(ModbusExceptionCode ex);
void assert_master_ex(ModbusExceptionCode ex);
void assert_expr(const std::string &message, bool expr);
void set_mode(const std::string &mode);
void set_request(const std::vector<int> &data);
void set_response(const std::vector<int> &data);
void set_reg_count(int n);
void set_coil_count(int n);
void clear_regs(int val);
void clear_coils(int val);
void set_rlock(int index, int lock);
void set_wlock(int index, int lock);
void reset();
void test_info(const std::string &s);
void run_test(const std::string &name, std::function<void()> f);

