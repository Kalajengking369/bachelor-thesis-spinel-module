#include "Application.h"

#include "ProtocolConstants.h"
#include "SpinelUtils.h"

#include <array>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

TxBuffer Application::onFrameReceived(const RxBuffer& rawRx, const UnwrappedSData& sdata,
                                      bool isAscii) {
    TxBuffer tx;
    std::lock_guard<std::mutex> lock(requestMtx);

    if(!request.isActive)
        return {};

    if(rawRx.flags & (RxFlags::OVERFLOW | RxFlags::CRC_ERROR | RxFlags::SHORT_FRAME)) {
        std::cerr << "Warning: Bad frame (flags=0x" << (int)rawRx.flags << ")\n";
        return {};
    }

    // Ignore responses from a different address, except broadcast and universal requests.
    if(request.addr != sdata.addr && request.addr != 0xFF && request.addr != 0xFE)
        return {};

    // Binary responses must match the signature of the pending request.
    if(!isAscii && sdata.sigValid && sdata.sig != request.sig) {
        std::cerr << "SIG Mismatch!\n";
        return {};
    }

    bool success = handlers.process(request.inst, sdata.payload, isAscii, sdata.addr);

    if(success && request.baudRate > 0) {
        // Baud rate is changed only after a valid response is received.
        tx.newBaud = request.baudRate;
    }

    request.isActive = false;
    lastResponseReceived = true;
    return tx;
}

bool Application::isRequestActive() {
    std::lock_guard<std::mutex> lock(requestMtx);
    return request.isActive;
}

void Application::onResponseSent(bool needReset) {
    (void)needReset;
}

void Application::sendBin(uint8_t inst, uint8_t addr, const uint8_t* data, size_t len,
                          uint32_t newBaud) {
    if(manager == nullptr)
        return;
    uint8_t sig = 0;

    {
        std::lock_guard<std::mutex> lock(requestMtx);
        if(request.isActive) {
            std::cerr << "Busy: ignoring instruction 0x" << std::hex << static_cast<int>(inst)
                      << "\n";
            return;
        }
        if(nextSig == 0)
            nextSig = 1;
        sig = nextSig++;
        request.isActive = true;
        lastResponseReceived = false;
        request.addr = addr;
        request.inst = inst;
        request.sig = sig;
        request.timestamp = std::chrono::steady_clock::now();
        request.baudRate = newBaud;
    }
    manager->sendRequest(addr, inst, sig, data, len);
}

void Application::sendAscii(uint8_t instBin, const char* instAscii, char addr, const char* data,
                            size_t len, uint32_t newBaud) {
    if(manager == nullptr)
        return;
    {
        std::lock_guard<std::mutex> lock(requestMtx);
        if(request.isActive) {
            std::cerr << "Busy.\n";
            return;
        }

        request.isActive = true;
        lastResponseReceived = false;
        request.addr = addr;
        request.inst = instBin;
        request.sig = 0;
        request.timestamp = std::chrono::steady_clock::now();
        request.baudRate = newBaud;
    }
    manager->sendRequest(addr, instAscii, data, len);
}

void Application::setManager(SpinelProtocolManager& mgr) {
    manager = &mgr;
}

void Application::checkResponseTimeouts() {
    std::lock_guard<std::mutex> lock(requestMtx);
    if(!request.isActive)
        return;
    auto now = std::chrono::steady_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - request.timestamp);

    if(elapsed.count() > Timeouts::RESPONSE_MS) {
        std::cerr << "TIMEOUT: Device " << (int)request.addr << " did not answer in time.\n";

        request.isActive = false;
        lastResponseReceived = false;
    }
}

void Application::reqTempBin(uint8_t addr) {
    sendBin(InstructionBin::GET_TEMP, addr);
}

void Application::reqTempAscii(char addr) {
    sendAscii(InstructionBin::GET_TEMP, InstructionAscii::GET_TEMP, addr);
}

void Application::reqRawBin(uint8_t addr) {
    sendBin(InstructionBin::GET_RAW, addr);
}

void Application::sendResetBin(uint8_t addr) {
    sendBin(InstructionBin::RESET, addr);
}

void Application::sendResetAscii(char addr) {
    sendAscii(InstructionBin::RESET, InstructionAscii::RESET, addr);
}

void Application::setCommsBin(uint8_t addr, uint8_t newAddr, uint32_t newBaud) {
    uint8_t speedCode = SpinelUtils::getSpinelSpeedCode(newBaud);
    if(speedCode == 0xFF) {
        std::cout << "Application Error";
        return;
    }

    uint8_t payload[] = {newAddr, speedCode};
    sendBin(InstructionBin::SET_COMMS, addr, payload, 2, newBaud);
}

void Application::setAddrAscii(char addr, char newAddr) {
    char payload[] = {newAddr};
    sendAscii(InstructionBin::SET_COMMS, InstructionAscii::SET_ADDR, addr, payload, 1);
}

void Application::setStatusBin(uint8_t addr, uint8_t status) {
    uint8_t payload[] = {status};
    sendBin(InstructionBin::SET_STATUS, addr, payload, 1);
}

void Application::setStatusAscii(char addr, char status) {
    char payload[] = {status};
    sendAscii(InstructionBin::SET_STATUS, InstructionAscii::SET_STATUS, addr, payload, 1);
}

void Application::setSpeedAscii(char addr, uint32_t newBaud) {
    uint8_t speedCode = SpinelUtils::getSpinelSpeedCode(newBaud);
    if(speedCode == 0xFF)
        return;

    char payload[] = {SpinelUtils::convertBinToAscii(speedCode)};
    sendAscii(InstructionBin::SET_COMMS, InstructionAscii::SET_SPEED, addr, payload, 1, newBaud);
}

void Application::enableConfigBin(uint8_t addr) {
    sendBin(InstructionBin::SET_CONFIG, addr);
}

void Application::enableConfigAscii(char addr) {
    sendAscii(InstructionBin::SET_CONFIG, InstructionAscii::SET_CONFIG, addr);
}

void Application::readCommsParams(uint8_t addr) {
    sendBin(InstructionBin::READ_COMMS, addr);
}

void Application::readStatusBin(uint8_t addr) {
    sendBin(InstructionBin::READ_STATUS, addr);
}

void Application::readStatusAscii(char addr) {
    sendAscii(InstructionBin::READ_STATUS, InstructionAscii::READ_STATUS, addr);
}

void Application::readInfoBin(uint8_t addr) {
    sendBin(InstructionBin::READ_INFO, addr);
}

void Application::readInfoAscii(char addr) {
    sendAscii(InstructionBin::READ_INFO, InstructionAscii::READ_INFO, addr);
}

void Application::writeUserDataBin(uint8_t addr, uint8_t pos, const uint8_t* data, size_t len) {
    if(len > 16)
        len = 16;
    std::array<uint8_t, 17> payload;
    payload[0] = pos;
    std::memcpy(&payload[1], data, len);
    sendBin(InstructionBin::WRITE_USER_DATA, addr, payload.data(), len + 1);
}

void Application::writeUserDataAscii(char addr, char pos, const char* data) {
    size_t len = std::strlen(data);
    if(len > 16)
        len = 16;

    std::array<char, 17> payload;
    payload[0] = pos;
    std::memcpy(&payload[1], data, len);

    sendAscii(InstructionBin::WRITE_USER_DATA, InstructionAscii::WRITE_USER_DATA, addr,
              payload.data(), len + 1);
}

void Application::readUserDataBin(uint8_t addr) {
    sendBin(InstructionBin::READ_USER_DATA, addr);
}

void Application::readUserDataAscii(char addr) {
    sendAscii(InstructionBin::READ_USER_DATA, InstructionAscii::READ_USER_DATA, addr);
}

void Application::readErrorsBin(uint8_t addr) {
    sendBin(InstructionBin::READ_ERRORS, addr);
}

void Application::readProdInfoBin(uint8_t addr) {
    sendBin(InstructionBin::READ_PROD_INFO, addr);
}

void Application::readChecksum(uint8_t addr) {
    sendBin(InstructionBin::READ_CHECKSUM, addr);
}

void Application::setChecksum(uint8_t addr, bool set) {
    uint8_t payload[] = {set};
    sendBin(InstructionBin::SET_CHECKSUM, addr, payload, 1);
}
