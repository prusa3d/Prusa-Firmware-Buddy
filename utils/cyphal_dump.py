#!/usr/bin/env -S python3 -u

import struct
from can import Bus as CanBus
from collections import defaultdict

_BIT_SRV_NOT_MSG = 1 << 25
_BIT_MSG_ANON = 1 << 24
_BIT_SRV_REQ = 1 << 24
_BIT_R23 = 1 << 23
_BIT_MSG_R7 = 1 << 7
SUBJECT_ID_MASK = 2**13 - 1
SERVICE_ID_MASK = 2**9 - 1
PRIORITY_MASK = 7
NODE_ID_MASK = 127


def port_id_to_name(port_id):
    return {
        21: 'prusa3d.ac_controller.Config.1',
        90: 'prusa3d.tool_offset_sensor.Config.1',
        100: 'prusa3d.nfc.command.AcceptEvent.1',
        101: 'prusa3d.nfc.command.Request.1',
        102: 'prusa3d.nfc.SetConfig.1',
        408: 'uavcan.file.Read.1',
        430: 'uavcan.node.GetInfo.1',
        435: 'uavcan.node.ExecuteCommand.1',
        500: 'prusa3d.common.metrics',
        501: 'prusa3d.common.Stats',
        502: 'prusa3d.common.AutoBootCancel',
        600: 'prusa3d.ac_controller.Status.1',
        601: 'prusa3d.common.leds.Config.1',
        900: 'prusa3d.tool_offset_sensor.Data_CH0.1',
        901: 'prusa3d.tool_offset_sensor.Status.1',
        902: 'prusa3d.tool_offset_sensor.Data_CH1.1',
        1000: 'prusa3d.nfc.event.Event.1',
        1001: 'prusa3d.nfc.Status.1',
        7509: 'uavcan.node.Heartbeat.1',
        7510: 'uavcan.node.port.List.1.0',
        8165: 'uavcan.pnp.NodeIDAllocationData.2',
        8166: 'uavcan.pnp.NodeIDAllocationData.1',
        8184: 'uavcan.diagnostic.Record.1',
    }.get(port_id, port_id)


def parse_frame(frame):
    identifier = frame.arbitration_id

    priority = (identifier >> 26) & PRIORITY_MASK
    source_node_id = identifier & NODE_ID_MASK
    if identifier & _BIT_SRV_NOT_MSG:
        if identifier & _BIT_R23:
            return None  # Wrong protocol

        service_id = (identifier >> 14) & SERVICE_ID_MASK
        request_not_response = identifier & _BIT_SRV_REQ != 0
        destination_node_id = (identifier >> 7) & NODE_ID_MASK
        what = 'Request' if request_not_response else 'Response'
        return {
            'time': frame.timestamp,
            'prio': priority,
            'what': what,
            'port': port_id_to_name(service_id),
            'src': source_node_id,
            'dst': destination_node_id,
            'data': frame.data,
        }

    if identifier & (_BIT_R23 | _BIT_MSG_R7):
        return None  # Wrong protocol

    subject_id = (identifier >> 8) & SUBJECT_ID_MASK
    return {
        'time': frame.timestamp,
        'prio': priority,
        'what': 'Message',
        'port': port_id_to_name(subject_id),
        'src': None if identifier & _BIT_MSG_ANON else source_node_id,
        'dst': None,
        'data': frame.data,
    }


ONGOING_TRANSFERS = defaultdict(list)


def unpack_bytes(data, n):
    return data[:n], data[n:]


def unpack_unsigned(data, n):
    value, data = unpack_bytes(data, n)
    return int.from_bytes(value, 'little', signed=False), data


def unpack_signed(data, n):
    value, data = unpack_bytes(data, n)
    return int.from_bytes(value, 'little', signed=True), data


def unpack_float(data):
    value, data = unpack_bytes(data, 4)
    return struct.unpack('f', value)[0], data


def unpack_version(data):
    major, data = unpack_unsigned(data, 1)
    minor, data = unpack_unsigned(data, 1)
    return f'{major}.{minor}', data


def unpack_pwm(data):
    return unpack_float(data)


def unpack_celsius(data):
    return unpack_float(data)


def unpack_target_temperature(data):
    tag, data = unpack_unsigned(data, 1)
    if tag == 0:
        return None, data
    else:
        return unpack_celsius(data)


def unpack_fan_state(data):
    pwm, data = unpack_pwm(data)
    rpm, data = unpack_float(data)
    status, data = unpack_signed(data, 1)
    #int8 STATUS_OK = 0
    #int8 STATUS_ERROR_STARTING = -1
    #int8 STATUS_ERROR_RUNNING = -2
    return f'pwm={pwm} rpm={rpm} status={status}', data


def unpack_union_target_temperature(data):
    """Unpack TargetTemperature union (Empty or Celsius)"""
    tag, data = unpack_unsigned(data, 1)
    if tag == 0:
        return 'not_set', data
    else:
        temp, data = unpack_celsius(data)
        return f'{temp}°C', data


def unpack_union_fan_config(data):
    """Unpack FanConfiguration union (Empty, PWM, or RPM)"""
    tag, data = unpack_unsigned(data, 1)
    if tag == 0:
        return 'automatic', data
    elif tag == 1:
        pwm, data = unpack_float(data)
        return f'pwm={pwm}', data
    elif tag == 2:
        rpm, data = unpack_float(data)
        return f'rpm={rpm}', data
    else:
        return f'unknown_tag={tag}', data


def unpack_union_vent_position(data):
    """Unpack VentPosition"""
    position, data = unpack_float(data)
    if position == 0:
        return 'inside', data
    elif position == -1:
        return 'outside', data
    else:
        return f'pos={position}', data


def unpack_union_rgbw_led_config(data):
    """Unpack RGBWLedConfiguration union (Empty or RGBW color + effect)"""
    tag, data = unpack_unsigned(data, 1)
    if tag == 0:
        return 'off', data
    else:
        # RGBW color: 4 uint8 values for R, G, B, W + effect uint8
        r, data = unpack_unsigned(data, 1)
        g, data = unpack_unsigned(data, 1)
        b, data = unpack_unsigned(data, 1)
        w, data = unpack_unsigned(data, 1)
        effect, data = unpack_unsigned(data, 1)

        effect_names = {0: 'STABLE', 1: 'SLOW_ON_1S'}
        effect_str = effect_names.get(effect, f'effect_{effect}')

        return f'rgbw=({r},{g},{b},{w}) {effect_str}', data


def unpack_shared_fault(data):
    """Unpack SharedFault.1.0 (uint32 bitmask)"""
    bitmask, data = unpack_unsigned(data, 4)
    fault_names = []
    if bitmask & (1 << 31):
        fault_names.append('UNKNOWN')
    if bitmask & (1 << 30):
        fault_names.append('HEARTBEAT_MISSING')
    if bitmask & (1 << 29):
        fault_names.append('DATA_TIMEOUT')
    if bitmask & (1 << 28):
        fault_names.append('PCB_OVERHEAT')
    if bitmask & (1 << 27):
        fault_names.append('MCU_OVERHEAT')

    # AC Controller specific faults (bits 0-16)
    ac_faults = []
    if bitmask & (1 << 0):
        ac_faults.append('RCD_TRIPPED')
    if bitmask & (1 << 1):
        ac_faults.append('POWERPANIC')
    if bitmask & (1 << 2):
        ac_faults.append('OVERHEAT')
    if bitmask & (1 << 3):
        ac_faults.append('PSU_FAN_NOK')
    if bitmask & (1 << 4):
        ac_faults.append('PSU_NTC_DISCONNECT')
    if bitmask & (1 << 5):
        ac_faults.append('PSU_NTC_SHORT')
    if bitmask & (1 << 6):
        ac_faults.append('BED_NTC_DISCONNECT')
    if bitmask & (1 << 7):
        ac_faults.append('BED_NTC_SHORT')
    if bitmask & (1 << 8):
        ac_faults.append('TRIAC_NTC_DISCONNECT')
    if bitmask & (1 << 9):
        ac_faults.append('TRIAC_NTC_SHORT')
    if bitmask & (1 << 10):
        ac_faults.append('BED_FAN0_NOK')
    if bitmask & (1 << 11):
        ac_faults.append('BED_FAN1_NOK')
    if bitmask & (1 << 12):
        ac_faults.append('TRIAC_FAN_NOK')
    if bitmask & (1 << 13):
        ac_faults.append('GRID_NOK')
    if bitmask & (1 << 14):
        ac_faults.append('BED_LOAD_NOK')
    if bitmask & (1 << 15):
        ac_faults.append('CHAMBER_LOAD_NOK')
    if bitmask & (1 << 16):
        ac_faults.append('PSU_NOK')
    if bitmask & (1 << 17):
        ac_faults.append('BED_RUNAWAY')

    all_faults = fault_names + ac_faults
    if all_faults:
        return f'0x{bitmask:08x}[{"|".join(all_faults)}]', data
    else:
        return f'0x{bitmask:08x}', data


def unpack_voltage_scalar(data):
    """Unpack uavcan.si.unit.voltage.Scalar.1.0 (float32 volt)"""
    volt, data = unpack_float(data)
    return f'{volt:.1f}V', data


def unpack_frequency_scalar(data):
    """Unpack uavcan.si.unit.frequency.Scalar.1.0 (float32 hertz)"""
    hertz, data = unpack_float(data)
    return f'{hertz:.1f}Hz', data


def unpack_power_scalar(data):
    """Unpack uavcan.si.unit.power.Scalar.1.0 (float32 watt)"""
    watt, data = unpack_float(data)
    return f'{watt:.1f}W', data


def unpack_subject_id_list(data):
    """Unpack SubjectIDList.1.0 union (mask, sparse_list, or total)"""
    tag, data = unpack_unsigned(data, 1)
    if tag == 0:
        # mask: bool[8192] - this would be 1024 bytes, likely not the case here
        # For now, skip the mask parsing and just indicate it's a mask
        return 'mask[...]', data  # This is complex to decode properly
    elif tag == 1:
        # sparse_list: SubjectID.1.0[<256]
        count, data = unpack_unsigned(data, 1)  # Array length
        subject_ids = []
        for _ in range(count):
            subject_id, data = unpack_unsigned(data, 2)  # SubjectID is uint16
            subject_ids.append(subject_id)
        return f'sparse[{",".join(map(str, subject_ids))}]', data
    elif tag == 2:
        # total: Empty
        return 'total', data
    else:
        return f'unknown_tag={tag}', data


def unpack_service_id_list(data):
    """Unpack ServiceIDList.1.0 (bool[512] mask)"""
    # ServiceIDList is a fixed-size bitmask of 512 bits = 64 bytes
    mask_bytes, data = unpack_bytes(data, 64)

    # Convert to list of active service IDs
    active_services = []
    for byte_idx, byte_val in enumerate(mask_bytes):
        for bit_idx in range(8):
            if byte_val & (1 << bit_idx):
                service_id = byte_idx * 8 + bit_idx
                active_services.append(service_id)

    if active_services:
        return f'services[{",".join(map(str, active_services))}]', data
    else:
        return 'services[]', data


def unpack_union(data, dispatch):
    tag, data = unpack_unsigned(data, 1)

    def default(data):
        return f'UNKNOWN tag={tag} data={data}', data

    return dispatch.get(tag, default)(data)


def unpack_nfc_event_id(data):
    event_id, data = unpack_unsigned(data, 2)
    return f'E#{event_id}', data


def unpack_nfc_request_id(data):
    request_id, data = unpack_unsigned(data, 2)
    return f'R#{request_id}', data


def unpack_nfc_tag_id(data):
    tag_id, data = unpack_unsigned(data, 1)
    return f'T#{tag_id}', data


def unpack_nfc_field(data):
    value, data = unpack_unsigned(data, 2)
    return f'F#{value}', data


def unpack_nfc_section(data):
    section, data = unpack_unsigned(data, 1)
    return {
        0: 'META',
        1: 'MAIN',
        2: 'AUXILIARY',
    }.get(section, section), data


def unpack_nfc_board_orientation(data):
    value, data = unpack_unsigned(data, 1)
    return {
        0: 'NORMAL',
        1: 'LEFT',
        2: 'RIGHT',
    }.get(value, value), data


def unpack_nfc_command_request_status(data):
    status, data = unpack_unsigned(data, 1)
    return {
        0: 'OK',
        1: 'REQUEST_QUEUE_FULL',
    }.get(status, status), data


def unpack_nfc_reader_error(data):
    error, data = unpack_unsigned(data, 1)
    pretty_error = {
        0: 'FIELD_NOT_PRESENT',
        1: 'WRONG_FIELD_TYPE',
        2: 'WRITE_PROTECTED',
        3: 'REGION_CORRUPT',
        4: 'TAG_INVALID',
        5: 'DATA_TOO_BIG',
        6: 'OTHER',
        7: 'NOT_IMPLEMENTED',
        254: 'RADIO_DISABLED',
        255: 'NO_ERROR',
    }.get(error, error)
    return f'error {pretty_error}', data


def unpack_nfc_value_type(data):
    value, data = unpack_unsigned(data, 1)
    return {
        0: 'BOOL_',
        1: 'INT32_',
        2: 'INT64_',
        3: 'FLOAT_',
        5: 'STRING',
        6: 'BYTES',
        7: 'UINT16_ARRAY',
    }.get(value, value), data


def unpack_nfc_tag_field(data):
    tag_id, data = unpack_nfc_tag_id(data)
    section, data = unpack_nfc_section(data)
    field, data = unpack_nfc_field(data)
    return f'tag_field {tag_id} {section} {field}', data


def unpack_nfc_request_data(data):

    def read_field(data):
        tag_field, data = unpack_nfc_tag_field(data)
        value_type, data = unpack_nfc_value_type(data)
        return f'read_field {tag_field} {value_type}', data

    def write_field(data):
        tag_field, data = unpack_nfc_tag_field(data)
        value, data = unpack_nfc_value(data)
        return f'write_field {tag_field} {value}', data

    def reset_state(data):
        return 'reset_state', data

    def set_params(data):
        mime_type_size, data = unpack_unsigned(data, 1)
        mime_type_data, data = unpack_bytes(data, mime_type_size)
        mime_type = pretty_string(mime_type_data)
        has_meta_region, data = unpack_unsigned(data, 1)
        return f'set_params mime={mime_type} has_meta={bool(has_meta_region)}', data

    def forget_tag(data):
        tag_id, data = unpack_nfc_tag_id(data)
        return f'forget_tag {tag_id}', data

    def enumerate_fields(data):
        tag_id, data = unpack_nfc_tag_id(data)
        section, data = unpack_nfc_section(data)
        return f'enumerate_fields {tag_id} {section}', data

    def raw_write(data):
        tag_id, data = unpack_nfc_tag_id(data)
        offset, data = unpack_unsigned(data, 2)
        data_size, data = unpack_unsigned(data, 1)
        raw_data, data = unpack_bytes(data, data_size)
        return f'raw_write {tag_id} offset={offset} data={raw_data.hex()}', data

    def raw_read(data):
        tag_id, data = unpack_nfc_tag_id(data)
        offset, data = unpack_unsigned(data, 2)
        num_bytes, data = unpack_unsigned(data, 2)
        return f'raw_read {tag_id} offset={offset} num_bytes={num_bytes}', data

    def initialize_tag(data):
        tag_id, data = unpack_nfc_tag_id(data)
        protect_first_num_bytes, data = unpack_unsigned(data, 2)
        password, data = unpack_bytes(data, 4)
        protection_policy, data = unpack_unsigned(data, 1)
        best_effort, data = unpack_unsigned(data, 1)
        policy_names = {0: 'NONE', 1: 'LOCK', 2: 'WRITE_PASSWORD'}
        policy = policy_names.get(protection_policy, protection_policy)
        return f'initialize_tag {tag_id} protect={protect_first_num_bytes} policy={policy} best_effort={bool(best_effort)}', data

    def unlock_tag(data):
        tag_id, data = unpack_nfc_tag_id(data)
        password, data = unpack_bytes(data, 4)
        return f'unlock_tag {tag_id} password={password.hex()}', data

    def enable_radio(data):
        return 'enable_radio', data

    def disable_radio(data):
        return 'disable_radio', data

    def set_config(data):
        enforce_antenna, data = unpack_unsigned(data, 1)
        auto_forget_tag, data = unpack_unsigned(data, 1)
        modulation_cnt, data = unpack_unsigned(data, 1)
        antenna = 'any' if enforce_antenna == 255 else f'A#{enforce_antenna}'
        return f'set_config antenna={antenna} auto_forget={bool(auto_forget_tag)} modulation_configs={modulation_cnt}', data

    def get_tag_uid(data):
        tag_id, data = unpack_nfc_tag_id(data)
        return f'get_tag_uid {tag_id}', data

    return unpack_union(
        data, {
            0: read_field,
            1: write_field,
            2: reset_state,
            3: set_params,
            4: forget_tag,
            5: enumerate_fields,
            6: raw_write,
            7: raw_read,
            8: initialize_tag,
            9: unlock_tag,
            10: enable_radio,
            11: disable_radio,
            12: set_config,
            13: get_tag_uid,
        })


def unpack_nfc_request_enumerate_fields_result(data):

    def unpack_nfc_enumerate_fields(data):
        cnt, data = unpack_unsigned(data, 1)
        fields = []
        for _ in range(cnt):
            value, data = unpack_unsigned(data, 2)
            fields.append(value)
        return f'enumerate_fields {fields}', data

    return unpack_union(data, {
        0: unpack_nfc_enumerate_fields,
        1: unpack_nfc_reader_error,
    })


def unpack_nfc_value(data):

    def unpack_bool(data):
        value, data = unpack_unsigned(data, 1)
        return f'bool={bool(value)}', data

    def unpack_int32(data):
        value, data = unpack_signed(data, 4)
        return f'int32={value}', data

    def unpack_int64(data):
        value, data = unpack_signed(data, 8)
        return f'int64={value}', data

    def unpack_float(data):
        value, data = unpack_bytes(data, 4)
        float_val = struct.unpack('<f', value)[0]
        return f'float={float_val}', data

    def unpack_nfc_string(data):
        string_size, data = unpack_unsigned(data, 1)
        string_data, data = unpack_bytes(data, string_size)
        string = pretty_string(string_data)
        return f'string {string}', data

    def unpack_nfc_bytes(data):
        bytes_size, data = unpack_unsigned(data, 1)
        bytes_data, data = unpack_bytes(data, bytes_size)
        return f'bytes={bytes_data.hex()}', data

    def unpack_uint16_array(data):
        array_size, data = unpack_unsigned(data, 1)
        values = []
        for _ in range(array_size):
            value, data = unpack_unsigned(data, 2)
            values.append(value)
        return f'uint16_array={values}', data

    return unpack_union(
        data, {
            0: unpack_bool,
            1: unpack_int32,
            2: unpack_int64,
            3: unpack_float,
            4: unpack_nfc_string,
            5: unpack_nfc_bytes,
            6: unpack_uint16_array,
        })


def unpack_nfc_value_or_error(data):
    return unpack_union(data, {
        0: unpack_nfc_value,
        1: unpack_nfc_reader_error,
    })


def unpack_nfc_raw_read_result(data):

    def unpack_raw_data(data):
        data_size, data = unpack_unsigned(data, 1)
        raw_data, data = unpack_bytes(data, data_size)
        return f'data={raw_data.hex()}', data

    return unpack_union(data, {
        0: unpack_raw_data,
        1: unpack_nfc_reader_error,
    })


def unpack_nfc_get_tag_uid_result(data):

    def unpack_uid(data):
        uid_size, data = unpack_unsigned(data, 1)
        uid_data, data = unpack_bytes(data, uid_size)
        return f'uid={uid_data.hex()}', data

    return unpack_union(data, {
        0: unpack_uid,
        1: unpack_nfc_reader_error,
    })


def unpack_nfc_request_result(data):

    def empty(data):
        return 'empty', data

    def read_field(data):
        value_or_error, data = unpack_nfc_value_or_error(data)
        return f'read_field {value_or_error}', data

    def write_field(data):
        error, data = unpack_nfc_reader_error(data)
        return f'write_field {error}', data

    def raw_read(data):
        result, data = unpack_nfc_raw_read_result(data)
        return f'raw_read {result}', data

    def raw_write(data):
        error, data = unpack_nfc_reader_error(data)
        return f'raw_write {error}', data

    def initialize_tag(data):
        error, data = unpack_nfc_reader_error(data)
        return f'initialize_tag {error}', data

    def unlock_tag(data):
        error, data = unpack_nfc_reader_error(data)
        return f'unlock_tag {error}', data

    def get_tag_uid(data):
        result, data = unpack_nfc_get_tag_uid_result(data)
        return f'get_tag_uid {result}', data

    return unpack_union(
        data, {
            0: empty,
            1: read_field,
            2: write_field,
            3: unpack_nfc_request_enumerate_fields_result,
            4: raw_read,
            5: raw_write,
            6: initialize_tag,
            7: unlock_tag,
            8: get_tag_uid,
        })


def unpack_nfc_event_data(data):

    def request_done(data):
        request_id, data = unpack_nfc_request_id(data)
        request_result, data = unpack_nfc_request_result(data)
        return f'request_done {request_id} {request_result}', data

    def tag_detected(data):
        tag_id, data = unpack_nfc_tag_id(data)
        antenna_id, data = unpack_unsigned(data, 1)
        return f'tag_detected {tag_id} A#{antenna_id}', data

    def tag_lost(data):
        tag_id, data = unpack_nfc_tag_id(data)
        return f'tag_lost {tag_id}', data

    return unpack_union(data, {
        0: request_done,
        1: tag_detected,
        2: tag_lost,
    })


def pretty_string(b):
    try:
        s = b.decode('utf-8')
        if s.isprintable():
            return f'"{s}"'
        else:
            raise Exception()
    except:
        s = b.hex()
        return f'0x{s}'


def pretty(port, what, data):
    if port == 'prusa3d.ac_controller.Config.1':
        if what == 'Request':
            bed_target_temp, data = unpack_union_target_temperature(data)
            chamber_target_temp, data = unpack_union_target_temperature(data)
            chamber_fan_config, data = unpack_union_fan_config(data)
            servo_config, data = unpack_union_vent_position(data)
            intake_vent, data = unpack_union_vent_position(data)
            psu_fan_config, data = unpack_union_fan_config(data)
            triac_fan_config, data = unpack_union_fan_config(data)
            external_fan_config, data = unpack_union_fan_config(data)
            rgb_led_strip, data = unpack_union_rgbw_led_config(data)
            splitter_enabled, data = unpack_unsigned(data, 1)

            return f'bed_temp={bed_target_temp} chamber_temp={chamber_target_temp} chamber_fan={chamber_fan_config} servo={servo_config} intake={intake_vent} psu_fan={psu_fan_config} triac_fan={triac_fan_config} ext_fan={external_fan_config} rgbw={rgb_led_strip} splitter={bool(splitter_enabled)}'

        elif what == 'Response':
            return 'config_ack'

    elif port == 'prusa3d.ac_controller.Status.1':
        # Bed
        bed_target_temp, data = unpack_target_temperature(data)
        bed_temp, data = unpack_celsius(data)
        bed_pwm, data = unpack_pwm(data)

        # Chamber
        chamber_target_temp, data = unpack_target_temperature(data)
        chamber_pwm, data = unpack_pwm(data)

        # Power supply
        psu_fan_state, data = unpack_fan_state(data)
        psu_temp, data = unpack_celsius(data)

        # Triac
        triac_fan_state, data = unpack_fan_state(data)
        triac_temp, data = unpack_celsius(data)

        # External fans
        external_fan0_state, data = unpack_fan_state(data)
        external_fan1_state, data = unpack_fan_state(data)

        # Board state
        board_temp, data = unpack_celsius(data)
        mcu_temp, data = unpack_celsius(data)
        supply_voltage, data = unpack_voltage_scalar(data)

        # RGB LED
        rgb_led_strip, data = unpack_union_rgbw_led_config(data)

        # Faults
        faults, data = unpack_shared_fault(data)

        # Time sync
        time_sync_precise, data = unpack_unsigned(data, 1)

        # AC power measurements
        ac_voltage, data = unpack_voltage_scalar(data)
        ac_frequency, data = unpack_frequency_scalar(data)
        bed_power, data = unpack_power_scalar(data)
        chamber_power, data = unpack_power_scalar(data)
        psu_power, data = unpack_power_scalar(data)

        return f'bed={bed_temp:.1f}°C({bed_target_temp}) bed_pwm={bed_pwm:.2f} bed_pwr={bed_power} chamber_pwm={chamber_pwm:.2f} chamber_pwr={chamber_power} psu={psu_fan_state} psu_temp={psu_temp:.1f}°C psu_pwr={psu_power} triac={triac_fan_state} triac_temp={triac_temp:.1f}°C ext_fan0={external_fan0_state} ext_fan1={external_fan1_state} board_temp={board_temp:.1f}°C mcu_temp={mcu_temp:.1f}°C supply={supply_voltage} rgb={rgb_led_strip} faults={faults} time_sync={bool(time_sync_precise)} ac={ac_voltage}@{ac_frequency}'

    elif port == 'prusa3d.nfc.Status.1':
        time_sync_precise, data = unpack_unsigned(data, 1)
        board_orientation, data = unpack_nfc_board_orientation(data)
        board_temp, data = unpack_celsius(data)
        mcu_temp, data = unpack_celsius(data)
        fs_1, data = unpack_unsigned(data, 2)
        fs_2, data = unpack_unsigned(data, 2)
        radio_enabled, data = unpack_unsigned(data, 1)
        faults, data = unpack_shared_fault(data)

        return f'time_sync_precise={bool(time_sync_precise)} board_orientation={board_orientation} board_temp={board_temp:.1f}°C mcu_temp={mcu_temp:.1f}°C fs_1={fs_1} fs_2={fs_2} radio_enabled={bool(radio_enabled)} faults={faults}'

    elif port == 'prusa3d.nfc.command.AcceptEvent.1':
        if what == 'Request':
            event_id, data = unpack_nfc_event_id(data)
            return f'{event_id}'

        elif what == 'Response':
            ok, data = unpack_unsigned(data, 1)
            return 'ok' if ok else 'nok'

    elif port == 'prusa3d.nfc.command.Request.1':
        if what == 'Request':
            request_id, data = unpack_nfc_request_id(data)
            request_data, data = unpack_nfc_request_data(data)
            return f'{request_id} {request_data}'

        elif what == 'Response':
            status, data = unpack_nfc_command_request_status(data)
            return f'{status}'

    elif port == 'prusa3d.nfc.event.Event.1':
        event_id, data = unpack_nfc_event_id(data)
        event_data, data = unpack_nfc_event_data(data)
        return f'{event_id} {event_data}'

    elif port == 'prusa3d.nfc.SetConfig.1':
        if what == 'Request':
            left_led_pwm, data = unpack_unsigned(data, 1)
            right_led_pwm, data = unpack_unsigned(data, 1)
            return f'left_led={left_led_pwm} right_led={right_led_pwm}'

        elif what == 'Response':
            return 'ack'

    elif port == 'uavcan.pnp.NodeIDAllocationData.2':
        node_id, data = unpack_unsigned(data, 2)
        unique_id_size, data = unpack_unsigned(data, 1)
        unique_id_data, data = unpack_bytes(data, unique_id_size)

        unique_id = pretty_string(unique_id_data)
        return f'node_id={node_id} unique_id={unique_id}'

    elif port == 'uavcan.diagnostic.Record.1':
        timestamp, data = unpack_unsigned(data, 7)
        severity, data = unpack_unsigned(data, 1)
        text_size, data = unpack_unsigned(data, 1)
        text_data, data = unpack_bytes(data, text_size)

        text = pretty_string(text_data)
        return f'{text}'

    elif port == 'uavcan.file.Read.1':
        if what == 'Request':
            offset, data = unpack_unsigned(data, 5)
            path_size, data = unpack_unsigned(data, 1)
            path_data, data = unpack_bytes(data, path_size)

            path = pretty_string(path_data)
            return f'offset={offset} path={path}'

        elif what == 'Response':
            error, data = unpack_unsigned(data, 2)
            payload_size, data = unpack_unsigned(data, 2)
            payload_data, data = unpack_bytes(data, payload_size)
            return f'error={error} size={payload_size}'

    elif port == 'uavcan.node.GetInfo.1':
        if what == 'Request':
            return f''

        elif what == 'Response':
            protocol_version, data = unpack_version(data)
            hardware_version, data = unpack_version(data)
            software_version, data = unpack_version(data)
            software_vcs_revision_id, data = unpack_unsigned(data, 8)
            unique_id, data = unpack_unsigned(data, 16)
            name_size, data = unpack_unsigned(data, 1)
            name_data, data = unpack_bytes(data, name_size)

            name = pretty_string(name_data)
            return f'proto={protocol_version} hw={hardware_version} sw={software_version} name={name}'

    elif port == 'uavcan.node.Heartbeat.1':
        uptime, data = unpack_unsigned(data, 4)
        health, data = unpack_unsigned(data, 1)
        mode, data = unpack_unsigned(data, 1)
        vssc, data = unpack_unsigned(data, 1)

        health = [
            'nominal ',
            'advisory',
            'caution ',
            'warning ',
        ][health]
        mode = [
            'operational    ',
            'initialization ',
            'maintenance    ',
            'software_update',
        ][mode]
        return f'{health} {mode} uptime={uptime} vssc={vssc}'

    elif port == 'uavcan.node.ExecuteCommand.1':
        if what == 'Request':
            command, data = unpack_unsigned(data, 2)
            parameter_size, data = unpack_unsigned(data, 1)
            parameter_data, data = unpack_bytes(data, parameter_size)

            parameter = pretty_string(parameter_data)

            if command == 0:
                return f'start_app'

            elif command == 1:
                return f'get_app_salted_hash {parameter}'

            elif command == 65533:
                return f'begin_software_update {parameter}'

            elif command == 65535:
                return f'restart'

            else:
                return f'command={command} parameter={parameter_data}'

        elif what == 'Response':
            status, data = unpack_unsigned(data, 1)
            output_size, data = unpack_unsigned(data, 1)
            output_data, data = unpack_bytes(data, output_size)

            status = [
                'success',
                'failure',
                'not_authorized',
                'bad_command',
                'bad_parameter',
                'bad_state',
                'internal_error',
            ][status]
            output = f' {pretty_string(output_data)}' if output_data else ''

            return f'{status}{output}'

    elif port in ('prusa3d.tool_offset_sensor.Data_CH0.1',
                  'prusa3d.tool_offset_sensor.Data_CH1.1'):
        sequence, data = unpack_unsigned(data, 1)
        # frequency is uint24, 16.8 fixed-point Hz
        freq_raw, data = unpack_unsigned(data, 3)
        freq_hz = freq_raw / 256.0
        sample, data = unpack_signed(data, 4)
        # deltas: dynamic array with 1-byte length prefix
        delta_count, data = unpack_unsigned(data, 1)
        deltas = []
        for _ in range(delta_count):
            d, data = unpack_signed(data, 2)
            deltas.append(d)
        # Reconstruct absolute sample values
        values = [sample & 0x0FFFFFFF]
        current = sample
        for d in deltas:
            current += d
            values.append(current & 0x0FFFFFFF)
        return f'seq={sequence} freq={freq_hz:.1f}Hz n={len(values)} samples={values}'

    elif port == 'prusa3d.tool_offset_sensor.Status.1':
        # SharedFault.1.0 (1 byte bitfield) + bool flags + sensor_errors
        faults, data = unpack_unsigned(data, 1)
        flags, data = unpack_unsigned(data, 1)
        time_sync = bool(flags & 0x01)
        ch0_active = bool(flags & 0x02)
        ch1_active = bool(flags & 0x04)
        sensor_fault = bool(flags & 0x08)
        sensor_errors, data = unpack_unsigned(data, 1)
        err_names = []
        if sensor_errors & 0x01: err_names.append('underrange')
        if sensor_errors & 0x02: err_names.append('overrange')
        if sensor_errors & 0x04: err_names.append('watchdog')
        if sensor_errors & 0x08: err_names.append('amp_high')
        if sensor_errors & 0x10: err_names.append('amp_low')
        if sensor_errors & 0x20: err_names.append('zero_count')
        errs = '|'.join(err_names) if err_names else 'none'
        return f'ch0={ch0_active} ch1={ch1_active} fault={sensor_fault} sync={time_sync} errors={errs}'

    elif port == 'prusa3d.tool_offset_sensor.Config.1':
        if what == 'Request':
            flags, data = unpack_unsigned(data, 1)
            ch0 = bool(flags & 0x01)
            ch1 = bool(flags & 0x02)
            return f'ch0={ch0} ch1={ch1}'
        elif what == 'Response':
            return 'ack'

    elif port == 'uavcan.node.port.List.1.0':
        # Network introspection - list of active ports
        return decode_port_list(data)

    else:
        return f'{len(data)}'


def decode_port_list(data):
    """Decode uavcan.node.port.List.1.0 message (network introspection)"""
    result = []

    # Publishers list: SubjectIDList.1.0
    publishers, data = unpack_subject_id_list(data)
    result.append(f'publishers: {publishers}')

    # Subscribers list: SubjectIDList.1.0
    subscribers, data = unpack_subject_id_list(data)
    result.append(f'subscribers: {subscribers}')

    # Clients list: ServiceIDList.1.0
    clients, data = unpack_service_id_list(data)
    result.append(f'clients: {clients}')

    # Servers list: ServiceIDList.1.0
    servers, data = unpack_service_id_list(data)
    result.append(f'servers: {servers}')

    return '\n    '.join(result)


TIME = None


def dump_internal(key, value):
    what, port, src, dst, transfer_id = key
    prio, data, time = value

    if what == 'Request':
        wha = 'req'
    elif what == 'Response':
        wha = 'res'
    elif what == 'Message':
        wha = 'msg'
    else:
        wha = 'unk'

    pretty_src = 'unk' if src is None else f'{src:>3}'
    pretty_dst = 'all' if dst is None else f'{dst:>3}'
    text = pretty(port, what, data)
    global TIME
    if TIME:
        nice_time = f'{time - TIME:11.6f}'
    else:
        nice_time = '   0.000000'
        TIME = time
    print(
        f'{nice_time} {prio} {wha} {pretty_src} -> {pretty_dst} {transfer_id:>2} {port} {text}'
    )


def dump_message(message):
    if not message:
        return

    # unpack message
    what = message.pop('what')
    port = message.pop('port')
    src = message.pop('src')
    dst = message.pop('dst')
    prio = message.pop('prio')
    data = message.pop('data')
    time = message.pop('time')

    # slice-off tail byte
    tail = data[-1]
    data = data[:-1]

    # unpack tail byte
    start_of_transfer = bool(tail & 0x80)
    end_of_transfer = bool(tail & 0x40)
    toggle = bool(tail & 0x20)  # ignored for now, we shouldn't need decoupling
    transfer_id = int(tail & 0x1f)

    key = what, port, src, dst, transfer_id
    value = prio, data, time

    if start_of_transfer and end_of_transfer:
        return dump_internal(key, value)

    # slice-off crc
    data = data[:-2]

    if start_of_transfer:
        ONGOING_TRANSFERS[key].append(value)
        return

    ONGOING_TRANSFERS[key].append(value)

    if end_of_transfer:
        prio = None
        data = b''
        time = None
        for part_prio, part_data, part_time in ONGOING_TRANSFERS.pop(key):
            prio = part_prio  # overwrite, whatever
            data = data + part_data
            time = part_time  # overwrite, whatever

        value = prio, data, time
        return dump_internal(key, value)


def main():
    with CanBus(interface='socketcan', channel='can0', fd=True) as bus:
        for frame in bus:
            message = parse_frame(frame)
            dump_message(message)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
