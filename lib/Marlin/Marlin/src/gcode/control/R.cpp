//! @file
//! @date Oct 25, 2019
//! @author Marek Bel
//!
//! see GcodeSuite::R()

#include "inc/MarlinConfig.h"

#if ENABLED(REDIRECT_GCODE_SUPPORT)

#include "../gcode.h"
#include "main.h"
#include "../../Marlin.h"
#include <cassert>

namespace
{
const unsigned long timeout = 5000; //!< No character received on UART after command was send timeout in ms.
//! There is ok in the end of Resend request. Disable this and change Prusa-Firmware so it doesn't send ok when
//! requesting line resend to make communication more robust.
//! As "Resend" is longer string than "ok" there is great risk, that "Resend"
//! becomes corrupted but "ok" readable making it false positive acknowledgment.
const bool ok_follows_resend = true;
long line_nr[2] = {}; //!< Line number, incremented each time redirected command is acknowledged.

enum class Machine_error : uint_least8_t
{
    Invalid, //!< Invalid machine (machine index out of range)
    Error,   //!< Uart HAL_ERROR
    Busy,    //!< Uart HAL_BUSY
    Timeout, //!< Timed out
};

enum class Send_error : uint_least8_t
{
    None = 0, //!< No error
    Recoverable,
    Irrecoverable,
};

class Parser
{
public:
    enum class Result : uint_least8_t
    {
        Incomplete,
        Ok,
        Resend,
    };
    Parser() : m_state(State::OR){}

    //! @brief Parse response
    //! @param response character received
    //! @retval Result::Incomplete no complete response received
    //! @retval Result::Ok "ok" received
    //! @retval Result::Resend "Resend" (and "ok" if ok_follows_resend) received
    Result parse (const char response)
    {
        switch(m_state)
        {
        case State::OR:
            if(response == 'o') m_state = State::K1;
            else if (response == 'R') m_state = State::E1;
            break;
        case State::K1:
            m_state = State::OR;
            if(response == 'k')
            {
                return Result::Ok;
            }
            break;
        case State::E1:
            if(response == 'e') m_state = State::S;
            else m_state = State::OR;
            break;
        case State::S:
            if(response == 's') m_state = State::E2;
            else m_state = State::OR;
            break;
        case State::E2:
            if(response == 'e') m_state = State::N;
            else m_state = State::OR;
            break;
        case State::N:
            if(response == 'n') m_state = State::D;
            else m_state = State::OR;
            break;
        case State::D:
            if (ok_follows_resend)
            {
                if(response == 'd') m_state = State::O;
                else m_state = State::OR;
            }
            else
            {
                if(response == 'd') return Result::Resend;
                else m_state = State::OR;
            }
            break;
        case State::O:
            if(response == 'o') m_state = State::K2;
            // intentionally stay there until 'o' received
            break;
        case State::K2:
            if(response == 'k') return Result::Resend;
            else m_state = State::O;
            break;
        }
        return Result::Incomplete;
    }

private:
    enum class State : uint_least8_t
    {
        OR, //!< initial state, waiting for o or R
        K1, //!< waiting for k
        E1, //!< waiting for e
        S,  //!< waiting for s
        E2, //!< waiting for e
        N,  //!< waiting for n
        D,  //!< waiting for d
        O,  //!< waiting for o
        K2, //!< waiting for k
    };
    State m_state;
};

} // anonymous namespace

//! @brief Signal R code failure
//! @param code (invalid) machine number
//! @param error see Error
//! @param receive
//!  * true receive response failed
//!  * false transmit failed
static void remote_machine_error(const uint_least8_t code, const Machine_error error, const bool receive = false)
{
    SERIAL_ECHO_START();
    SERIAL_CHAR('R');
    SERIAL_ECHO(int(code));
    SERIAL_CHAR(' ');
    if (error == Machine_error::Error ||
            error == Machine_error::Busy ||
            error == Machine_error::Timeout)
    {
        if (receive)
        {
            SERIAL_ECHO(MSG_RECEIVE);
        }
        else
        {
            SERIAL_ECHO(MSG_SEND);
        }

    }

    switch (error)
    {
    case Machine_error::Invalid:
        SERIAL_ECHOLNPGM(MSG_INVALID_MACHINE);
        break;
    case Machine_error::Error:
        SERIAL_ECHOLNPGM(MSG_UART_ERROR);
        break;
    case Machine_error::Busy:
        SERIAL_ECHOLNPGM(MSG_UART_BUSY);
        break;
    case Machine_error::Timeout:
        SERIAL_ECHOLNPGM(MSG_UART_TIMED_OUT);
        break;

    }
}

//! @brief Transmit N[line_nr] command*[checksum]\n
//! @param command gcode to send
//! @param uartrxbuff uart associated with uartrxbuff is used to transmit
//! @retval HAL_OK Succeeded.
//! @retval HAL_ERROR Failed.
//! @retval HAL_BUSY Failed.
//! @retval HAL_TIMEOUT Failed.
static HAL_StatusTypeDef transmit(const char *command, UART_HandleTypeDef *uart, const uint8_t machine_index)
{
    char buffer[16];
    int buffer_strlen = snprintf(buffer, sizeof(buffer), "N%ld ", line_nr[machine_index]);
    assert(buffer_strlen > 0);
    assert(static_cast<size_t>(buffer_strlen) <= sizeof(buffer));
    uint8_t checksum = 0;

    for(uint_least16_t i = 0; i < buffer_strlen; ++i) checksum = checksum^buffer[i];

    HAL_StatusTypeDef retval = HAL_UART_Transmit( uart, reinterpret_cast<uint8_t*>(buffer), buffer_strlen, HAL_MAX_DELAY);
    const uint16_t command_len = strlen(command);
    if (retval == HAL_OK) retval = HAL_UART_Transmit( uart, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(command)), command_len, HAL_MAX_DELAY);

    for(uint_least16_t i = 0; i < command_len; ++i) checksum = checksum^command[i];

    buffer_strlen = snprintf(buffer, sizeof(buffer), "*%d\n", checksum);
    assert(buffer_strlen > 0);
    assert(static_cast<size_t>(buffer_strlen) <= sizeof(buffer));
    if (retval == HAL_OK) retval = HAL_UART_Transmit(uart, reinterpret_cast<uint8_t*>(buffer), buffer_strlen, HAL_MAX_DELAY);
    return retval;
}

static void clear_input_buffer(uartrxbuff_t &uartrxbuff)
{
    while(uartrxbuff_getchar(&uartrxbuff) >= 0);
}

//! @brief send command to uartrxbuff connected machine and wait for acknowledge
//! @param command gcode command (null terminated string)
//! @param uartrxbuff UART associated with uartrxbuff is used to transmit and receive
//! @param machine_index used for error reporting and keeping track of last line number
//! @retval Send_error::None Succes
//! @retval Send_error::Recoverable No response received or asked for re-send
//! @retval Send_error::Irrecoverable command empty or unable to transmit
static Send_error send(const char *command, UART_HandleTypeDef* uart, uartrxbuff_t &uartrxbuff, const uint8_t machine_index)
{
    HAL_StatusTypeDef ret;
    if (command[0])
    {
        clear_input_buffer(uartrxbuff);
        ret = transmit(command, uart, machine_index);
    }
    else return Send_error::Irrecoverable;;

    switch(ret)
    {
    case HAL_OK:
        break;
    case HAL_ERROR:
        remote_machine_error(machine_index, Machine_error::Error, false);
        return Send_error::Irrecoverable;
    case HAL_BUSY:
        remote_machine_error(machine_index, Machine_error::Busy, false);
        return Send_error::Irrecoverable;
    case HAL_TIMEOUT:
        remote_machine_error(machine_index, Machine_error::Timeout, false);
        return Send_error::Irrecoverable;
    }

    Parser parser;
    uint32_t last_response = millis();
    while (true)
    {
        int recieved = uartrxbuff_getchar(&uartrxbuff);
        SERIAL_CHAR(recieved);
        if (recieved >= 0)
        {
            last_response = millis();
            Parser::Result parse_result = parser.parse(recieved);
            if (parse_result == Parser::Result::Ok)
            {
                ++line_nr[machine_index];
                return Send_error::None;
            }
            else if (parse_result == Parser::Result::Resend)
            {
                return Send_error::Recoverable;
            }
        }
        else
        {
            if((millis() - last_response) > timeout )
            {
                remote_machine_error(machine_index, Machine_error::Timeout, true);
                return Send_error::Recoverable;
            }
            idle(true);
        }
    }
}

//! @brief Reliable send
//!
//! Adds line number and checksum to parser.string_arg and sends it to UART associated with uartrxbuff.
//! If no response or "Resend" request is received, it sends command once again. If still no acknowledgment
//! is received, it tries to reset local and remote line number counter and send again.
//! Maximum number of tries is 7.
//!
//! Known limitations:
//!
//! Situation when printer asks for Resend, but word "Resend" in response is corrupted but "ok" in the
//! end of response is not corrupted leads to wrong assumption, that command was successfully acknowledged.
//! This state could be detected by line number mismatch when next command is sent, but to simplify
//! implementation and avoid need to remember previous command it is ignored and line number counter is reset
//! instead.
//!
//! @param uartrxbuff UART associated with uartrxbuff is used to transmit and receive
//! @param machine_index used for error reporting and keeping track of last line number
static void reliable_send(UART_HandleTypeDef* uart, uartrxbuff_t &uartrxbuff, const uint8_t machine_index)
{
    enum class State : uint_least8_t
    {
        Send, //!< initial state
        Resend,
        Reset_line,
    };
    State state = State::Send;

    for(uint_least8_t i = 0; i < 8; ++i)
    {
        switch(state)
        {
        case State::Send:
            switch(send(parser.string_arg, uart, uartrxbuff, machine_index))
            {
            case Send_error::None:
            case Send_error::Irrecoverable:
                return;
            case Send_error::Recoverable:
                state = State::Resend;
                break;
            }
            break;
        case State::Resend:
            switch(send(parser.string_arg, uart, uartrxbuff, machine_index))
            {
            case Send_error::None:
            case Send_error::Irrecoverable:
                return;
            case Send_error::Recoverable:
                state = State::Reset_line;
                break;
            }
            break;
        case State::Reset_line:
            line_nr[machine_index] = 0;
            switch(send("M110 N0", uart, uartrxbuff, machine_index))
            {
            case Send_error::None:
                state = State::Send;
                break;
            case Send_error::Irrecoverable:
                return;
            case Send_error::Recoverable:
                break;
            }
            break;

        }
    }
}


//! @brief Redirect gcode to another machine connected to UART
//!
//! Command blocks until "ok" is received from connected machine
//! It is not a good idea to embed line number and checksum inside redirected
//! command, as redirector adds these internally.
//!
//! @param machine_index machine number

void GcodeSuite::R(const uint8_t machine_index)
{
    int constexpr case1 = 1;
    switch (machine_index)
    {
    case case1:
    {
        static_assert(case1 < (sizeof(line_nr)/sizeof(line_nr[0])), "line_nr array too small");
        reliable_send(&huart1, uart1rxbuff, machine_index);
        break;
    }
    default:
        remote_machine_error(machine_index, Machine_error::Invalid);
        break;
    }
}

#endif //ENABLED(REDIRECT_GCODE_SUPPORT)

