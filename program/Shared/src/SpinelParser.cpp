#include "SpinelParser.h"

#include "SpinelUtils.h"
using Protocol::CR;
using Protocol::PRE;

/*
 * Valid frames:
 * ASCII: PRE + FRM + ADR + INST + DATA + CR
 * BIN:   PRE + FRM + NUM + NUM + ADR + SIG + INST + DATA + SUM + CR
 * NUM  specifies number of bytes from ADR to CR, including CR.
 */

bool SpinelParser::processByte(uint8_t byte)
{

    switch (rxState)
    {
    case RxState::WAIT_PRE:
        if (byte == PRE)
        {
            rxBuffer.clear();
            rxBuffer.append(byte);
            currentSum = byte;
            rxState = RxState::WAIT_FRM;
        }
        else
        {
            incError();
        }
        break;

    case RxState::WAIT_FRM:
        if (SpinelUtils::isAsciiFrame(byte))
        {
            if (byte == static_cast<uint8_t>(Protocol::ASCII66))
            {
                rxBuffer.append(byte);
                rxState = RxState::WAIT_ASCII_SDATA;
                break;
            }
            // Unsupported ASCII format, ignore until CR.
            rxState = RxState::WAIT_FOR_CR_IGNORE;
            break;
        }
        rxBuffer.append(byte);
        validFormat = (byte == Protocol::BIN97);
        currentSum += byte;
        rxState = RxState::WAIT_NUM_1;
        break;

    case RxState::WAIT_NUM_1:
        rxBuffer.append(byte);
        currentSum += byte;
        rxState = RxState::WAIT_NUM_2;
        break;

    case RxState::WAIT_NUM_2:
        rxBuffer.append(byte);
        currentSum += byte;
        expectedLen = (static_cast<uint16_t>(rxBuffer[2]) << 8) | rxBuffer[3];

        if (expectedLen == 0)
        {
            resetParser();
            break;
        }
        // Invalid format is skipped by NUM, short valid frame is returned with a flag.
        if (!validFormat || expectedLen < 5)
        {
            incError();
            if (validFormat && expectedLen > 0)
            {
                rxBuffer.flags |= RxFlags::SHORT_FRAME;
                rxState = RxState::WAIT_SHORT_FRAME_ADDR;
            }
            else
            {
                rxState = RxState::SUB_NUM;
            }
            break;
        }

        rxState = RxState::WAIT_BIN_SDATA;
        break;

    case RxState::WAIT_SHORT_FRAME_ADDR:
        rxBuffer.append(byte);
        expectedLen--;
        if (expectedLen == 0)
        {
            rxState = RxState::WAIT_PRE;
            return true;
        }
        break;

    case RxState::WAIT_BIN_SDATA:
        expectedLen--;
        rxBuffer.append(byte);
        if (expectedLen > 1)
        {
            currentSum += byte;
        }
        if (expectedLen == 1)
        { // Current byte is SUM, next byte must be CR.
            if (0xFF - currentSum != byte)
            {
                rxBuffer.flags |= RxFlags::CRC_ERROR;
                incError();
            }
            rxState = RxState::WAIT_FOR_CR_FINALIZE;
        }
        break;

    case RxState::WAIT_FOR_CR_FINALIZE:
        rxBuffer.append(byte);
        if (byte == CR)
        {
            rxState = RxState::WAIT_PRE;
            return true;
        }
        incError();
        resetParser();
        break;

    case RxState::WAIT_ASCII_SDATA:
        rxBuffer.append(byte);
        if (byte == CR)
        {
            rxState = RxState::WAIT_PRE;
            return true;
        }
        break;

    case RxState::WAIT_FOR_CR_IGNORE:
        if (byte == CR)
        {
            resetParser();
        }
        break;

    case RxState::SUB_NUM:
        if (expectedLen > 0 && --expectedLen == 0)
        {
            resetParser();
        }
        break;
    }

    return false;
}

void SpinelParser::handleTimeout()
{
    if (!isReceiving())
        return;

    if (rxState != RxState::WAIT_PRE)
    {
        incError();
        resetParser();
    }
}

void SpinelParser::resetParser()
{
    rxState = RxState::WAIT_PRE;
    rxBuffer.clear();
    expectedLen = 0;
    validFormat = false;
    currentSum = 0;
}

volatile uint8_t &SpinelParser::getErrorCount()
{
    return commErrors;
}

bool SpinelParser::isReceiving() const
{
    return (rxState != RxState::WAIT_PRE);
}

void SpinelParser::incError()
{
    if (commErrors < 255)
        commErrors++;
}

const RxBuffer &SpinelParser::getParsedFrame() const
{
    return rxBuffer;
}