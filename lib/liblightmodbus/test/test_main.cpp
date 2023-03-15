#include "tester.hpp"
using namespace std::string_literals;

void last_register_tests()
{
	run_test("Reading last coils/registers", [](){
		set_mode("pdu");
		for (int i = 1; i <= 4; i++)
		{
			build_request({1, i, 65535, 1});
			dump_request();
			parse_request();
			assert_slave_ok();
			parse_response();
			assert_master_ok();
			dump_data();
		}
	});

	run_test("Reading two last coils/registers", [](){
		for (int i = 1; i <= 4; i++)
		{
			set_mode("pdu");
			build_request({1, i, 65534, 2});
			dump_request();
			parse_request();
			assert_slave_ok();
			parse_response();
			assert_master_ok();
			dump_data();
		}
	});

	run_test("Reading one past the last coil/register", [](){
		set_mode("pdu");
		for (int i = 1; i <= 4; i++)
		{
			build_request({1, i, 65535, 2});
			assert_master_err(MODBUS_GENERAL_ERROR(RANGE));

			set_request({i, 255, 255, 0, 2});
			dump_request();
			parse_request();
			dump_slave();
			assert_slave_ex(MODBUS_EXCEP_ILLEGAL_ADDRESS);
		}
	});
}

void max_read_tests()
{
	run_test("Reading 2000 coils/discrete inputs", [](){
		set_mode("pdu");
		for (int i = 1; i <= 2; i++)
		{
			build_request({1, i, 0, 2000});
			assert_master_ok();
			parse_request();
			dump_slave();
			assert_slave_ex(MODBUS_EXCEP_NONE);
			parse_response();
			assert_master_ok();
		}
	});

	run_test("Reading 2001 coils/discrete inputs", [](){
		set_mode("pdu");
		for (int i = 1; i <= 2; i++)
		{
			build_request({1, i, 0, 2001});
			dump_master();
			assert_master_err(MODBUS_GENERAL_ERROR(COUNT));

			set_request({i, 0, 0, 0x07, 0xd1});
			parse_request();
			dump_slave();
			assert_slave_ex(MODBUS_EXCEP_ILLEGAL_VALUE);
			parse_response();
			assert_master_ok();
		}
	});

	run_test("Reading 125 input/holding registers", [](){
		set_mode("pdu");
		for (int i = 3; i <= 4; i++)
		{
			build_request({1, i, 0, 125});
			assert_master_ok();
			parse_request();
			dump_slave();
			assert_slave_ex(MODBUS_EXCEP_NONE);
			parse_response();
			assert_master_ok();
		}
	});

	run_test("Reading 126 input/holding registers", [](){
		set_mode("pdu");
		for (int i = 3; i <= 4; i++)
		{
			build_request({1, i, 0, 126});
			dump_master();
			assert_master_err(MODBUS_GENERAL_ERROR(COUNT));

			set_request({i, 0, 0, 0x00, 0x74});
			parse_request();
			dump_slave();
			assert_slave_ex(MODBUS_EXCEP_ILLEGAL_VALUE);
			parse_response();
			assert_master_ok();
		}
	});
}

void short_frames_tests()
{
	auto short_slave_request_test = [](const std::string &mode){
		run_test("Parse empty "s + mode + " request frame"s, [mode](){
			set_mode(mode);
			set_request({});
			parse_request();
			assert_slave_err(MODBUS_REQUEST_ERROR(LENGTH));
		});
	};

	auto short_master_request_test = [](const std::string &mode){
		run_test("Master parse empty "s + mode + " request frame"s, [mode](){
			set_mode(mode);
			set_request({});
			set_response({1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
			parse_response();
			assert_master_err(MODBUS_REQUEST_ERROR(LENGTH));
		});
	};

	auto short_master_response_test = [](const std::string &mode){
		run_test("Master parse empty "s + mode + " response frame"s, [mode](){
			set_mode(mode);
			build_request({1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
			set_response({});
			parse_response();
			assert_master_err(MODBUS_RESPONSE_ERROR(LENGTH));
		});
	};

	short_slave_request_test("pdu");
	short_slave_request_test("rtu");
	short_slave_request_test("tcp");

	short_master_request_test("pdu");
	short_master_request_test("rtu");
	short_master_request_test("tcp");

	short_master_response_test("pdu");
	short_master_response_test("rtu");
	short_master_response_test("tcp");
}

void modbus_pdu_tests()
{
	run_test("[PDU] build an exception", [](){
		set_mode("pdu");
		build_request({1, 1, 3, 4});
		assert_master_ok();
		build_exception(1, 1, MODBUS_EXCEP_ACK);
		dump_response();
		assert_slave_ex(MODBUS_EXCEP_ACK);
		assert_slave_ok();
		parse_response();
		assert_master_ok();
		assert_master_ex(MODBUS_EXCEP_ACK);
	});
}

void modbus_rtu_tests()
{
	run_test("Parse a raw Modbus RTU request (write register)", [](){
		set_mode("rtu");
		set_request({0x01, 0x06, 0xab, 0xcd, 0x01, 0x23, 0x78, 0x58});
		parse_request();
		assert_slave_ok();
		assert_reg(0xabcd, 0x0123);
		dump_response();
		parse_response();
		assert_master_ok();
	});

	run_test("Parse a raw Modbus RTU request with invalid CRC (big-endian)", [](){
		set_mode("rtu");
		set_request({0x01, 0x06, 0xab, 0xcd, 0x01, 0x23, 0x58, 0x78});
		parse_request();
		assert_slave_err(MODBUS_REQUEST_ERROR(CRC));
	});

	run_test("Parse a raw Modbus RTU request with invalid CRC", [](){
		set_mode("rtu");
		set_request({0x01, 0x06, 0xab, 0xcd, 0x01, 0x23, 0xfa, 0xce});
		parse_request();
		assert_slave_err(MODBUS_REQUEST_ERROR(CRC));
	});

	run_test("Parse a raw Modbus RTU response (exception)", [](){
		build_request({1, 1, 0, 1});
		set_response({1, 0x81, 1, 0x81, 0x90});
		parse_response();
		dump_master();
		assert_master_ok();
	});

	run_test("Parse a raw Modbus RTU response (exception) with invalid CRC", [](){
		build_request({1, 1, 0, 1});
		set_response({1, 0x81, 1, 0xff, 0xff});
		parse_response();
		dump_master();
		assert_master_err(MODBUS_RESPONSE_ERROR(CRC));
	});

	run_test("Broadcast frame with invalid CRC", [](){
		set_mode("rtu");
		set_request({0, 1, 0, 0, 0, 4, 0xff, 0xff});
		parse_request();
		dump_slave();
		assert_slave_err(MODBUS_REQUEST_ERROR(CRC));
		assert_slave_ex(MODBUS_EXCEP_NONE);
	});

	run_test("Cause slave exception in request 01", [](){
		set_mode("rtu");
		set_request({1, 1, 0, 0, 0, 0, 0x3c, 0x0a});
		parse_request();
		dump_slave();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_VALUE);
	});

	run_test("Slave exception to broadcast request 01", [](){
		set_mode("rtu");
		set_request({0, 1, 0, 0, 0, 0, 0x3d, 0xdb});
		parse_request();
		dump_slave();
		dump_response();
		assert_expr("no response", response_data.empty());
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
	});

	run_test("Check response to broadcast read request", [](){
		set_mode("rtu");
		build_request({0, 1, 0, 4});
		parse_request();
		dump_slave();
		dump_response();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		assert_expr("no response", response_data.empty());
	});

	run_test("Broadcast write request", [](){
		set_mode("rtu");
		build_request({0, 6, 77, 88});
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		assert_reg(77, 88);
		assert_expr("no response", response_data.empty());

	});

	run_test("Write request for other slave", [](){
		set_mode("rtu");
		build_request({33, 6, 55, 44});
		assert_master_ok();
		parse_request();
		assert_slave_err(MODBUS_REQUEST_ERROR(ADDRESS));
		assert_reg(55, 0);
		assert_expr("no response", response_data.empty());
	});

	run_test("Exception during broadcast write request", [](){
		set_mode("rtu");
		set_wlock(55, 1);
		build_request({0, 6, 55, 44});
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		assert_reg(55, 0);
		assert_expr("no response", response_data.empty());
	});

	run_test("Mismatched addres in request/response", [](){
		set_mode("rtu");
		build_request({1, 1, 3, 4});
		assert_master_ok();
		parse_request();
		assert_slave_ok();

		build_request({2, 1, 3, 4});
		assert_master_ok();

		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(ADDRESS));
	});

	run_test("[RTU] bad response CRC", [](){
		set_mode("rtu");
		build_request({1, 1, 3, 4});
		set_response({0, 1, 0, 0, 0, 4, 0xff, 0xff});
		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(CRC));
	});

	run_test("[RTU] build an exception", [](){
		set_mode("rtu");
		build_request({1, 1, 3, 4});
		assert_master_ok();
		build_exception(1, 1, MODBUS_EXCEP_SLAVE_FAILURE);
		dump_response();
		assert_slave_ex(MODBUS_EXCEP_SLAVE_FAILURE);
		assert_slave_ok();
		parse_response();
		assert_master_ok();
		assert_master_ex(MODBUS_EXCEP_SLAVE_FAILURE);
	});

	run_test("[RTU] attempt to build a broadcast exception", [](){
		set_mode("rtu");
		assert_master_ok();
		build_exception(0, 33, MODBUS_EXCEP_ACK);
		dump_response();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		assert_slave_err(MODBUS_GENERAL_ERROR(ADDRESS));
	});
}

void modbus_tcp_tests()
{
	run_test("[TCP] Write a register", [](){
		set_mode("tcp");
		build_request({1, 6, 0xaabb, 0xccdd});
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		assert_reg(0xaabb, 0xccdd);
	});

	run_test("[TCP] Bad protocol ID", [](){
		set_mode("tcp");
		set_request({0, 0, 0, 1, 0, 3, 1, 0xaa, 0xbb});
		parse_request();
		assert_slave_err(MODBUS_REQUEST_ERROR(BAD_PROTOCOL));

		set_request({0, 0, 0, 1, 0, 3, 1, 0xaa, 0xbb});
		set_response({0, 0, 0, 0, 0, 3, 1, 0xaa, 0xbb});
		parse_response();
		assert_master_err(MODBUS_REQUEST_ERROR(BAD_PROTOCOL));

		set_request({0, 0, 0, 0, 0, 3, 1, 0xaa, 0xbb});
		set_response({0, 0, 1, 0, 0, 3, 1, 0xaa, 0xbb});
		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(BAD_PROTOCOL));
	});

	run_test("[TCP] Mismatched transaction ID", [](){
		set_mode("tcp");
		set_request({55, 55, 0, 0, 0, 3, 1, 0xaa, 0xbb});
		set_response({53, 55, 0, 0, 0, 3, 1, 0xaa, 0xbb});
		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(BAD_TRANSACTION));
	});

	run_test("[TCP] Invalid declared length", [](){
		set_mode("tcp");
		set_request({0, 0, 0, 0, 0xff, 0x03, 1, 0xaa, 0xbb});
		parse_request();
		assert_slave_err(MODBUS_REQUEST_ERROR(LENGTH));

		set_request({0, 0, 0, 0, 0x03, 0x03, 1, 0xcc, 0xdd});
		set_response({0, 0, 0, 0, 0, 3, 1, 0xaa, 0xbb});
		parse_response();
		assert_master_err(MODBUS_REQUEST_ERROR(LENGTH));

		set_request({0, 0, 0, 0, 0, 0x03, 1, 0xcc, 0xdd});
		set_response({0, 0, 0, 0, 0xff, 0xff, 1, 0xaa, 0xbb});
		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(LENGTH));
	});

	run_test("[TCP] Valid request/response", [](){
		set_mode("tcp");
		build_request({1, 6, 77, 88});
		parse_request();
		assert_slave_ok();
		parse_response();
		assert_master_ok();
		assert_reg(77, 88);
	});

	run_test("[TCP] Address mismatch", [](){
		set_mode("tcp");
		set_request({0, 0, 0, 0, 0, 6, 1, 6, 0, 7, 0, 8});
		set_response({0, 0, 0, 0, 0, 6, 2, 6, 0, 7, 0, 8});
		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(ADDRESS));
		assert_reg(7, 0);
	});

	run_test("[TCP] Check ignoring Unit ID", [](){
		set_mode("tcp");
		set_request({0, 0, 0, 0, 0, 6, 0xff, 6, 0, 7, 0, 8});
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		assert_reg(7, 8);
	});

	run_test("[TCP] build an exception", [](){
		set_mode("tcp");
		build_request({1, 1, 3, 4});
		dump_request();
		assert_master_ok();
		build_exception(1, 1, MODBUS_EXCEP_NACK);
		dump_response();
		assert_slave_ex(MODBUS_EXCEP_NACK);
		assert_slave_ok();
		parse_response();
		assert_master_ok();
		assert_master_ex(MODBUS_EXCEP_NACK);
	});

	run_test("[TCP] build an exception with diffent slaveID", [](){
		set_mode("tcp");
		build_request({1, 1, 3, 4});
		dump_request();
		assert_master_ok();
		build_exception(0xff, 1, MODBUS_EXCEP_NACK);
		dump_response();
		assert_slave_ex(MODBUS_EXCEP_NACK);
		assert_slave_ok();
		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(ADDRESS));
	});
}

void single_write_tests()
{
	run_test("Write a coil to 1 and 0", [](){
		set_mode("pdu");
		build_request({1, 5, 0xc001, 0xbeef});
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		assert_coil(0xc001, 1);

		build_request({1, 5, 0xc001, 0x0000});
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		assert_coil(0xc001, 0);
	});

	run_test("Write a register", [](){
		set_mode("pdu");
		build_request({1, 6, 0xdead, 0xface});
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		assert_reg(0xdead, 0xface);
	});

	run_test("Write a write-protected register", [](){
		set_mode("pdu");
		build_request({1, 6, 0xdead, 0xface});
		set_wlock(0xdead, 1);
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_SLAVE_FAILURE);
		assert_reg(0xdead, 0x0000);
	});

	run_test("Write the last register", [](){
		set_mode("pdu");
		build_request({1, 6, 0xffff, 0xaaaa});
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		assert_reg(0xffff, 0xaaaa);
	});

	run_test("Write request with extra byte", [](){
		set_mode("pdu");
		set_request({6, 0, 33, 44, 55, 0xfa});
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_VALUE);
		assert_reg(0xffff, 0x0000);
	});

	run_test("Write request with one byte missing", [](){
		set_mode("pdu");
		set_request({6, 0xff, 0xff, 44});
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_VALUE);
		assert_reg(0xffff, 0x0000);
	});

	run_test("Coil write request with bad coil value", [](){
		set_mode("pdu");
		set_request({5, 0xff, 0xff, 0, 1});
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_VALUE);
		assert_reg(0xffff, 0x0000);
	});
}

void multiple_write_tests()
{
	run_test("Write 4 registers", [](){
		set_mode("pdu");
		set_reg_count(0x0103);
		build_request({1, 16, 0x00ff, 4, 17, 18, 19, 20});
		dump_request();
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		dump_queries();
		assert_reg(0x00ff, 17);
		assert_reg(0x0102, 20);
		parse_response();
		assert_master_ok();
	});

	run_test("[16] Write 4 registers, broadcast", [](){
		set_mode("rtu");
		set_reg_count(0x0103);
		build_request({0, 16, 0x00ff, 4, 17, 18, 19, 20});
		dump_request();
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		assert_expr("no response", response_data.empty());
		assert_reg(0x00ff, 17);
		assert_reg(0x0102, 20);
	});

	run_test("Attempt to write 0 registers", [](){
		set_mode("pdu");
		set_reg_count(2);
		build_request({1, 16, 0x0001, 0});
		assert_master_err(MODBUS_GENERAL_ERROR(COUNT));

		set_request({16, 0, 1, 0, 0, 0});
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_VALUE);
		parse_response();
		assert_master_ok();
	});

	run_test("Attempt to write registers with invalid byte count", [](){
		set_mode("pdu");
		set_reg_count(1);
		set_request({16, 0, 0, 0, 1, 3, 0xff, 0xff});
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_VALUE);
		parse_response();
		assert_master_ok();
	});

	run_test("Attempt to wrap address range when writing registers", [](){
		set_mode("pdu");
		set_request({16, 0xff, 0xff, 0, 2, 4, 0xff, 0xff, 0xee, 0xee});
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_ADDRESS);
		parse_response();
		assert_master_ok();
		assert_reg(0xffff, 0);
		assert_reg(0x0000, 0);
	});

	run_test("Write last register", [](){
		set_mode("pdu");
		build_request({1, 16, 0xffff, 1, 0xdead});
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		parse_response();
		assert_master_ok();
		assert_reg(0xffff, 0xdead);
	});

	run_test("Write 8 coils", [](){
		set_mode("pdu");
		build_request({1, 15, 0x00ff, 8, 1, 0, 1, 0, 1, 0, 1, 0});
		dump_request();
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		dump_queries();
		assert_coil(0x00ff, 1);
		assert_coil(0x0102, 0);
		parse_response();
		assert_master_ok();
	});

	run_test("Attempt to write 0 coils", [](){
		set_mode("pdu");
		set_coil_count(2);
		build_request({1, 15, 0x0001, 0});
		assert_master_err(MODBUS_GENERAL_ERROR(COUNT));

		set_request({15, 0, 1, 0, 0, 0});
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_VALUE);
		parse_response();
		assert_master_ok();
	});

	run_test("Attempt to write coils with invalid byte count", [](){
		set_mode("pdu");
		set_coil_count(1);
		set_request({15, 0, 0, 0, 9, 1, 0xff});
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_VALUE);
		parse_response();
		assert_master_ok();
	});

	run_test("Attempt to wrap address range when writing coils", [](){
		set_mode("pdu");
		set_request({15, 0xff, 0xff, 0, 2, 1, 0xff});
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_ADDRESS);
		parse_response();
		assert_master_ok();
		assert_coil(0xffff, 0);
		assert_coil(0x0000, 0);
	});

	run_test("Write last coil", [](){
		set_mode("pdu");
		build_request({1, 15, 0xffff, 1, 1});
		assert_master_ok();
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		parse_response();
		assert_master_ok();
		assert_coil(0xffff, 1);
	});

	run_test("[15] too short frame", [](){
		set_mode("pdu");
		set_coil_count(1);
		set_request({15, 0, 0, 0, 9, 1});
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_VALUE);
		parse_response();
		assert_master_ok();
	});
}

void mask_write_test()
{
	run_test("[22] Mask write register", [](){
		set_mode("pdu");
		set_reg_count(1);
		build_request({1, 22, 0, 0xf2f2, 0x2525});
		regs.at(0) = 0x1212;
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		dump_regs();
		assert_reg(0, 0x1717);
		parse_response();
		assert_master_ok();
	});

	run_test("[22] Mask write register with one extra byte", [](){
		set_mode("pdu");
		set_reg_count(1);
		set_request({22, 0, 0, 11, 22, 22, 33, 44});
		regs.at(0) = 0x1212;
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_VALUE);
		assert_reg(0, 0x1212);
	});

	run_test("[22] Mask write register broadcast", [](){
		set_mode("rtu");
		set_reg_count(1);
		build_request({0, 22, 0, 0xf2f2, 0x2525});
		regs.at(0) = 0x1212;
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_NONE);
		assert_expr("no response", response_data.empty());
		dump_regs();
		assert_reg(0, 0x1717);
	});
}

void illegal_function_test()
{
	run_test("Call function 0x7f on slave", [](){
		set_mode("pdu");
		set_request({0x7f, 0xde, 0xad, 0xbe, 0xef});
		parse_request();
		assert_slave_ok();
		assert_slave_ex(MODBUS_EXCEP_ILLEGAL_FUNCTION);
	});

	run_test("Mismatched request/response function", [](){
		set_mode("pdu");
		build_request({1, 1, 3, 4});
		assert_master_ok();
		parse_request();
		assert_slave_ok();

		build_request({1, 2, 3, 4});
		assert_master_ok();

		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(FUNCTION));
	});

	run_test("Parse response to function 0x7f on master", [](){
		set_mode("pdu");
		set_request({0x7f, 0xde, 0xad, 0xbe, 0xef});
		set_response({0x7f, 0xc0, 0xff, 0xee, 0x33});
		parse_response();
		assert_master_err(MODBUS_GENERAL_ERROR(FUNCTION));
	});
}

void invalid_response_tests()
{
	run_test("[03 resp] Invalid declared data count", [](){
		set_mode("pdu");
		build_request({1, 3, 4, 3});
		assert_master_ok();

		set_response({3, 5, 0, 0, 0, 0, 0, 0});
		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(LENGTH));
	});

	run_test("[03 resp] No data in response", [](){
		set_mode("pdu");
		build_request({1, 3, 4, 3});
		assert_master_ok();

		set_response({3, 0, 0});
		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(LENGTH));
	});

	run_test("[03 req] 0 registers", [](){
		set_mode("pdu");
		set_request({3, 0, 0, 0, 0});
		set_response({3, 2, 0, 3});
		parse_response();
		assert_master_err(MODBUS_REQUEST_ERROR(COUNT));
	});

	run_test("[03 req] 126 registers", [](){
		set_mode("pdu");
		set_request({3, 0, 0, 0, 126});
		set_response({3, 0, 7});
		parse_response();
		assert_master_err(MODBUS_REQUEST_ERROR(COUNT));
	});

	run_test("[03 req] address range wrap", [](){
		set_mode("pdu");
		build_request({1, 3, 0xffff, 1});
		parse_request();
		assert_slave_ok();
		parse_response();
		assert_master_ok();

		set_request({3, 0xff, 0xff, 0x00, 0x02});
		parse_response();
		assert_master_err(MODBUS_REQUEST_ERROR(RANGE));
	});

	run_test("[01 req] 2001 coils", [](){
		set_mode("pdu");
		set_request({1, 0, 0, 0x07, 0xd1});
		set_response({1, 255, 7});
		parse_response();
		assert_master_err(MODBUS_REQUEST_ERROR(COUNT));
	});

	run_test("[06 resp] Index mismatch", [](){
		set_mode("pdu");
		build_request({1, 6, 15, 16});
		set_response({6, 15, 0, 0, 16});
		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(INDEX));
	});

	run_test("[06 resp] Value mismatch", [](){
		set_mode("pdu");
		build_request({1, 6, 15, 16});
		set_response({6, 0, 15, 0, 26});
		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(VALUE));
	});

	run_test("[15 resp] Index mismatch", [](){
		set_mode("pdu");
		build_request({1, 15, 0, 2, 1, 1});
		set_response({15, 0, 1, 0, 2});
		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(INDEX));
	});

	run_test("[15 resp] Count mismatch", [](){
		set_mode("pdu");
		build_request({1, 15, 0, 2, 1, 1});
		set_response({15, 0, 0, 0, 3});
		parse_response();
		assert_master_err(MODBUS_RESPONSE_ERROR(COUNT));
	});

	run_test("[15 resp] bad declared requst length", [](){
		set_mode("pdu");
		set_request({15, 0, 0, 0, 2, 1, 0xff, 0});
		set_response({15, 0, 0, 0, 2});
		parse_response();
		assert_master_err(MODBUS_REQUEST_ERROR(LENGTH));
	});

	run_test("[15 resp] Address range wrap", [](){
		set_mode("pdu");
		set_request({15, 0xff, 0xff, 0, 2, 1, 0xff});
		set_response({15, 0xff, 0xff, 0, 2});
		parse_response();
		assert_master_err(MODBUS_REQUEST_ERROR(RANGE));
	});
}

void test_main()
{
	modbus_pdu_tests();
	modbus_rtu_tests();
	modbus_tcp_tests();
	short_frames_tests();

	illegal_function_test();
	single_write_tests();
	multiple_write_tests();
	mask_write_test();

	last_register_tests();
	max_read_tests();
	invalid_response_tests();
}