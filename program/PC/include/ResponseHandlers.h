#pragma once
#include "SpinelBuffers.h"

#include <array>
#include <vector>

class ResponseHandlers {
  private:
    using HandlerFn = bool (ResponseHandlers::*)(SpinelBufferView payload, bool isAscii,
                                                 uint8_t addr);

    struct HandlerEntry {
        uint8_t inst;
        HandlerFn handler;
    };

    static const std::array<HandlerEntry, 16> handlerTable;

    bool handleGetTemp(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleGetRaw(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleReset(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleSetStatus(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleEnableConfig(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleSetComms(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleReadComms(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleReadStatus(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleReadInfo(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleWriteUserData(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleReadUserData(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleReadErrors(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleReadProdInfo(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleReadChecksum(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleSetChecksum(SpinelBufferView payload, bool isAscii, uint8_t addr);
    bool handleUnknown(SpinelBufferView payload, bool isAscii, uint8_t addr);

    bool isAckOK(uint8_t ackByte, bool isAscii);
    void printError(uint8_t ackByte, bool isAscii, const char* contextMsg, uint8_t addr);

    std::vector<uint8_t> detectedAddrs;

  public:
    const std::vector<uint8_t>& getDetectedAddresses() const {
        return detectedAddrs;
    }
    void clearDetectedAddresses() {
        detectedAddrs.clear();
    }
    bool process(uint8_t origInst, SpinelBufferView payload, bool isAscii, uint8_t addr);
};