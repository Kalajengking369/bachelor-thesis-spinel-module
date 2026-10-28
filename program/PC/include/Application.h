#pragma once
#include "ISpinelApplication.h"
#include "ResponseHandlers.h"
#include "SpinelProtocolManager.h"

#include <chrono>
#include <mutex>
#include <vector>

class Application : public ISpinelApplication {
  private:
    struct PendingRequest {
        bool isActive = false;
        uint8_t addr = 0;
        uint8_t inst = 0;
        uint8_t sig = 0;
        std::chrono::steady_clock::time_point timestamp;
        uint32_t baudRate = 0;
    };

    SpinelProtocolManager* manager = nullptr;

    std::mutex requestMtx;
    PendingRequest request;
    ResponseHandlers handlers;
    uint8_t nextSig = 1;
    bool lastResponseReceived = false;

    TxBuffer onFrameReceived(const RxBuffer& rawRx, const UnwrappedSData& sdata,
                             bool isAscii) override;
    void onResponseSent(bool needReset) override;

  public:
    void sendBin(uint8_t inst, uint8_t addr, const uint8_t* data = nullptr, size_t len = 0,
                 uint32_t newBaud = 0);
    void sendAscii(uint8_t instBin, const char* instAscii, char addr, const char* data = nullptr,
                   size_t len = 0, uint32_t newBaud = 0);
    Application() = default;
    void setManager(SpinelProtocolManager& manager);
    void checkResponseTimeouts();

    std::vector<uint8_t> getDetectedAddresses() {
        return handlers.getDetectedAddresses();
    }
    void clearDetectedAddresses() {
        handlers.clearDetectedAddresses();
    }

    bool wasLastResponseReceived() const {
        return lastResponseReceived;
    }
    bool isRequestActive();

    void reqTempBin(uint8_t addr);
    void reqTempAscii(char addr);
    void reqRawBin(uint8_t addr);
    void sendResetBin(uint8_t addr);
    void sendResetAscii(char addr);

    void setCommsBin(uint8_t addr, uint8_t newAddr, uint32_t newBaud);
    void setAddrAscii(char addr, char newAddr);
    void setStatusBin(uint8_t addr, uint8_t status);
    void setStatusAscii(char addr, char status);
    void setSpeedAscii(char addr, uint32_t newBaud);

    void enableConfigBin(uint8_t addr);
    void enableConfigAscii(char addr);
    void readCommsParams(uint8_t addr);
    void readStatusBin(uint8_t addr);
    void readStatusAscii(char addr);
    void readInfoBin(uint8_t addr);
    void readInfoAscii(char addr);
    void writeUserDataBin(uint8_t addr, uint8_t pos, const uint8_t* data, size_t len);
    void writeUserDataAscii(char addr, char pos, const char* data);
    void readUserDataBin(uint8_t addr);
    void readUserDataAscii(char addr);
    void readErrorsBin(uint8_t addr);
    void readProdInfoBin(uint8_t addr);
    void readChecksum(uint8_t addr);
    void setChecksum(uint8_t addr, bool set);
};
