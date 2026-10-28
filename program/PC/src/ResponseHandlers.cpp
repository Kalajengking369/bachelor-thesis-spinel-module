#include "ResponseHandlers.h"

#include "ProtocolConstants.h"
#include "SpinelUtils.h"

#include <iomanip>
#include <iostream>
#include <sstream>

const std::array<ResponseHandlers::HandlerEntry, 16> ResponseHandlers::handlerTable = {{
    {InstructionBin::GET_TEMP, &ResponseHandlers::handleGetTemp},
    {InstructionBin::GET_RAW, &ResponseHandlers::handleGetRaw},
    {InstructionBin::RESET, &ResponseHandlers::handleReset},
    {InstructionBin::SET_STATUS, &ResponseHandlers::handleSetStatus},
    {InstructionBin::SET_CONFIG, &ResponseHandlers::handleEnableConfig},
    {InstructionBin::SET_COMMS, &ResponseHandlers::handleSetComms},
    {InstructionBin::READ_COMMS, &ResponseHandlers::handleReadComms},
    {InstructionBin::READ_STATUS, &ResponseHandlers::handleReadStatus},
    {InstructionBin::READ_INFO, &ResponseHandlers::handleReadInfo},
    {InstructionBin::WRITE_USER_DATA, &ResponseHandlers::handleWriteUserData},
    {InstructionBin::READ_USER_DATA, &ResponseHandlers::handleReadUserData},
    {InstructionBin::READ_ERRORS, &ResponseHandlers::handleReadErrors},
    {InstructionBin::READ_CHECKSUM, &ResponseHandlers::handleReadChecksum},
    {InstructionBin::SET_CHECKSUM, &ResponseHandlers::handleSetChecksum},
    {InstructionBin::READ_PROD_INFO, &ResponseHandlers::handleReadProdInfo},
}};

bool ResponseHandlers::process(uint8_t origInst, SpinelBufferView payload, bool isAscii,
                               uint8_t addr) {
    if(payload.isEmpty()) {
        std::cerr << "Error: Missing ACK byte.\n";
        return false;
    }

    HandlerFn handlerToCall = &ResponseHandlers::handleUnknown;

    // Select handler according to the original request instruction.
    for(const auto& entry : handlerTable) {
        if(entry.inst == origInst) {
            handlerToCall = entry.handler;
            break;
        }
    }

    return (this->*handlerToCall)(payload, isAscii, addr);
}

bool ResponseHandlers::isAckOK(uint8_t ackByte, bool isAscii) {
    if(isAscii) {
        return ackByte == AckAscii::OK;
    } else {
        return ackByte == AckBin::OK;
    }
}

void ResponseHandlers::printError(uint8_t ackByte, bool isAscii, const char* contextMsg,
                                  uint8_t addr) {
    std::cerr << "Error [" << contextMsg << "] from Device ";
    if(isAscii)
        std::cerr << "'" << (char)addr << "'";
    else
        std::cerr << (int)addr;

    std::cerr << ": MCU answered ";

    uint8_t binCode = ackByte;
    if(isAscii) {
        // Convert ASCII ACK to binary code to use one shared switch.
        binCode = SpinelUtils::convertAsciiToBin(ackByte);
    }

    switch(binCode) {
    case AckBin::ERROR:
        std::cerr << "Generic Error";
        break;
    case AckBin::INVALID_INSTRUCTION:
        std::cerr << "Invalid Instruction";
        break;
    case AckBin::INVALID_DATA:
        std::cerr << "Invalid Data";
        break;
    case AckBin::UNAUTHORIZED_WRITE:
        std::cerr << "Unauthorized Write";
        break;
    case AckBin::DEVICE_FAILURE:
        std::cerr << "Device Failure (Hardware error/Unavailable)";
        break;
    default:
        std::cerr << "Code 0x" << std::hex << (int)ackByte << std::dec;
        break;
    }
    std::cerr << "\n";
}

bool ResponseHandlers::handleGetTemp(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(isAckOK(ack, isAscii)) {
        if(isAscii)
            std::cout << "Device '" << (char)addr << "' Temperature: ";
        else
            std::cout << "Device " << (int)addr << " Temperature: ";

        if(isAscii) {
            if(payload.len > 1) {
                for(size_t i = 1; i < payload.len; i++) {
                    std::cout << (char)payload[i];
                }
                std::cout << std::endl;
            } else {
                std::cout << "No text data" << std::endl;
            }
        } else {
            if(payload.len >= 3) {
                uint16_t uRaw = (static_cast<uint16_t>(payload[1]) << 8) | payload[2];
                int16_t rawValue = static_cast<int16_t>(uRaw);
                float temp = rawValue / 32.0f; // Temperature is encoded in 1/32 °C steps.
                std::cout << std::fixed << std::setprecision(1) << temp << " C" << std::endl;
            } else {
                std::cout << "Incomplete binary data" << std::endl;
            }
        }
    } else {
        printError(ack, isAscii, "GET_TEMP", addr);
        return false;
    }
    return true;
}

bool ResponseHandlers::handleGetRaw(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(!isAckOK(ack, isAscii)) {
        printError(ack, isAscii, "GET_RAW", addr);
        return false;
    }

    if(payload.len < 3) {
        std::cout << "Error: Invalid data len\n";
        return false;
    }

    uint16_t rawValue = (static_cast<uint16_t>(payload[1]) << 8) | payload[2];

    if(isAscii)
        std::cout << "Device '" << (char)addr << "' ";
    else
        std::cout << "Device " << (int)addr << " ";

    std::cout << "Sensor RAW Value: " << rawValue << " (0x" << std::hex << rawValue << std::dec
              << ")\n";

    return true;
}

bool ResponseHandlers::handleReset(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(isAckOK(ack, isAscii)) {
        if(isAscii)
            std::cout << "Device '" << (char)addr << "' Reset Command Acknowledged.\n";
        else
            std::cout << "Device " << (int)addr << " Reset Command Acknowledged.\n";
    } else {
        printError(ack, isAscii, "RESET", addr);
        return false;
    }
    return true;
}

bool ResponseHandlers::handleSetStatus(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(isAckOK(ack, isAscii)) {
        if(isAscii)
            std::cout << "Device '" << (char)addr << "' Status Byte Set Successfully.\n";
        else
            std::cout << "Device " << (int)addr << " Status Byte Set Successfully.\n";
    } else {
        printError(ack, isAscii, "SET_STATUS", addr);
        return false;
    }
    return true;
}

bool ResponseHandlers::handleEnableConfig(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(isAckOK(ack, isAscii)) {
        if(isAscii)
            std::cout << "Device '" << (char)addr << "' Configuration Enabled.\n";
        else
            std::cout << "Device " << (int)addr << " Configuration Enabled.\n";
    } else {
        printError(ack, isAscii, "SET_CONFIG", addr);
        return false;
    }
    return true;
}

bool ResponseHandlers::handleSetComms(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(isAckOK(ack, isAscii)) {
        if(isAscii)
            std::cout << "Device '" << (char)addr << "' Communication Settings Changed.\n";
        else
            std::cout << "Device " << (int)addr << " Communication Settings Changed.\n";
    } else {
        printError(ack, isAscii, "SET_COMM", addr);
        return false;
    }
    return true;
}

bool ResponseHandlers::handleReadComms(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(isAckOK(ack, isAscii)) {
        if(payload.len < 3) {
            std::cerr << "Error: Incomplete payload for Read Comms.\n";
            return false;
        }
        if(isAscii)
            std::cout << "Device '" << (char)addr << "' Params - ";
        else
            std::cout << "Device " << (int)addr << " Params - ";

        std::cout << "Address: " << (payload[1]) << std::dec << ", ";
        std::cout << "Speed: " << SpinelUtils::getUartSpeed(payload[2]) << std::endl;
    } else {
        printError(ack, isAscii, "READ_COMM_PARAMS", addr);
        return false;
    }
    return true;
}

bool ResponseHandlers::handleReadStatus(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];
    if(!isAckOK(ack, isAscii)) {
        printError(ack, isAscii, "READ_STATUS", addr);
        return false;
    }

    if(payload.len < 2) {
        std::cout << "Error: Empty status\n";
        return false;
    }

    uint8_t status = payload[1];

    if(isAscii)
        std::cout << "Device '" << (char)addr << "' Status: ";
    else
        std::cout << "Device " << (int)addr << " Status: ";

    if(isAscii) {
        std::cout << static_cast<char>(status) << "\n";
    } else {
        std::cout << "0x" << std::hex << static_cast<int>(status) << std::dec << "\n";
    }
    return true;
}

bool ResponseHandlers::handleReadInfo(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];
    if(!isAckOK(ack, isAscii)) {
        printError(ack, isAscii, "READ_INFO", addr);
        return false;
    }

    size_t startIdx = 1;
    if(isAscii && payload.len > 1 && payload[1] == ' ') {
        startIdx = 2;
    }

    if(payload.len <= startIdx) {
        std::cout << "Device Info: Empty string\n";
        return true;
    }

    std::string infoStr(reinterpret_cast<const char*>(payload.data + startIdx),
                        payload.len - startIdx);

    // Discovery response contains entries in the "slot/address" format.
    if(infoStr.find('/') != std::string::npos) {
        std::cout << "--- Detected Devices List ---\n";
        std::stringstream ss(infoStr);
        std::string segment;
        bool found = false;

        while(ss >> segment) {
            size_t slashPos = segment.find('/');
            if(slashPos != std::string::npos) {
                std::string index = segment.substr(0, slashPos);
                std::string addressStr = segment.substr(slashPos + 1);
                std::cout << "  Device Slot: " << index << " | Spinel Address: " << addressStr
                          << "\n";

                // Store detected device address for later commands.
                if(!addressStr.empty()) {
                    detectedAddrs.push_back(static_cast<uint8_t>(addressStr[0]));
                }
                found = true;
            }
        }
        if(!found)
            std::cout << "  No active devices reported.\n";
        std::cout << "-----------------------------\n";
    } else {
        if(isAscii)
            std::cout << "Device '" << (char)addr << "' Info: " << infoStr << "\n";
        else
            std::cout << "Device " << (int)addr << " Info: " << infoStr << "\n";
    }

    return true;
}

bool ResponseHandlers::handleWriteUserData(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(isAckOK(ack, isAscii)) {
        if(isAscii)
            std::cout << "Device '" << (char)addr << "' User Data Written Successfully.\n";
        else
            std::cout << "Device " << (int)addr << " User Data Written Successfully.\n";
    } else {
        printError(ack, isAscii, "WRITE_USER_DATA", addr);
        return false;
    }
    return true;
}

bool ResponseHandlers::handleReadUserData(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(!isAckOK(ack, isAscii)) {
        printError(ack, isAscii, "READ_USER_DATA", addr);
        return false;
    }

    if(isAscii)
        std::cout << "Device '" << (char)addr << "' User Data: ";
    else
        std::cout << "Device " << (int)addr << " User Data: ";

    if(payload.len <= 1) {
        std::cout << "Empty data\n";
        return true;
    }
    for(size_t i = 1; i < payload.len; i++) {
        std::cout << (char)payload[i];
    }
    std::cout << "\n";

    return true;
}

bool ResponseHandlers::handleReadErrors(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(!isAckOK(ack, isAscii)) {
        printError(ack, isAscii, "READ_ERRORS", addr);
        return false;
    }

    if(payload.len >= 2) {
        if(isAscii)
            std::cout << "Device '" << (char)addr << "' ";
        else
            std::cout << "Device " << (int)addr << " ";
        std::cout << "Communication Errors: " << static_cast<int>(payload[1]) << "\n";
        return true;
    } else {
        std::cout << "Error: Incomplete Binary payload\n";
        return false;
    }
}

bool ResponseHandlers::handleReadProdInfo(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(!isAckOK(ack, isAscii)) {
        printError(ack, isAscii, "READ_PROD_INFO", addr);
        return false;
    }

    if(payload.len < 9) {
        std::cout << "Error: Invalid data length\n";
        return false;
    }

    if(isAscii)
        std::cout << "=== Device '" << (char)addr << "' Production Info ===\n";
    else
        std::cout << "=== Device " << (int)addr << " Production Info ===\n";

    std::cout << "Info: ";
    for(size_t i = 1; i <= 8 && i < payload.len; ++i) {
        std::cout << static_cast<char>(payload[i]);
    }
    std::cout << "\n=======================\n";

    return true;
}

bool ResponseHandlers::handleReadChecksum(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(!isAckOK(ack, isAscii)) {
        printError(ack, isAscii, "READ_CHECKSUM", addr);
        return false;
    }

    if(payload.len < 2) {
        std::cout << "Error: Invalid data length\n";
        return false;
    }

    if(isAscii)
        std::cout << "Device '" << (char)addr << "' Checksum: ";
    else
        std::cout << "Device " << (int)addr << " Checksum: ";

    std::cout << (payload[1] ? "ENABLED" : "NOT ENABLED") << "\n";
    return true;
}

bool ResponseHandlers::handleSetChecksum(SpinelBufferView payload, bool isAscii, uint8_t addr) {
    uint8_t ack = payload[0];

    if(!isAckOK(ack, isAscii)) {
        printError(ack, isAscii, "SET_CHECKSUM", addr);
        return false;
    }

    if(isAscii)
        std::cout << "Device '" << (char)addr << "' CRC set correctly\n";
    else
        std::cout << "Device " << (int)addr << " CRC set correctly\n";

    return true;
}

bool ResponseHandlers::handleUnknown(SpinelBufferView, bool isAscii, uint8_t addr) {
    std::cerr << "Received response from Device ";
    if(isAscii)
        std::cerr << "'" << (char)addr << "'";
    else
        std::cerr << (int)addr;
    std::cerr << " for unknown instruction.\n";
    return false;
}