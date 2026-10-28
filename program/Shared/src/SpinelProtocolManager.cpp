#include "SpinelProtocolManager.h"

SpinelProtocolManager::SpinelProtocolManager(IFrameQueue &queue, IUartLayer &uart,
                                             ISpinelApplication &app)
    : rxQueue(queue), uart(uart), app(app) {}

void SpinelProtocolManager::run()
{
    RxBuffer rx;
    while (true)
    {
        if (rxQueue.pop(rx))
        {
            processFrame(rx);
        }
        else
        {
            break;
        }
    }
}

void SpinelProtocolManager::processFrame(const RxBuffer &rx)
{
    // Unwrap frame down to application payload.
    UnwrappedFrame frame = frameLayer.unwrapFrame(rx.getView());
    if (!frame.frameValid)
        return;

    UnwrappedSData sdata = sdataLayer.unwrapSData(frame.payload, frame.isAscii);
    if (!sdata.addrValid)
        return;

    TxBuffer response = app.onFrameReceived(rx, sdata, frame.isAscii);
    // Send response if requested by the application.
    if (response.hasResponse)
    {
        uint8_t sig = sdata.sigValid ? sdata.sig : 0;
        sendResponse(response.addr, response.ackCode, sig, response.data.data(), response.len,
                     frame.isAscii);
    }
    // Change UART speed only after the response is sent.
    if (response.newBaud > 0)
    {
        uart.setSpeed(response.newBaud);
    }

    // Run delayed actions after the response leaves the device.
    if (response.triggerFlashSave || response.triggerReset)
    {
        app.onResponseSent(response.triggerReset);
    }
}

void SpinelProtocolManager::sendRequest(uint8_t addr, uint8_t inst, uint8_t sig,
                                        const uint8_t *data, size_t len)
{
    if (len > 247)
        return;
    // Wrap payload with binary Spinel protocol bytes.
    std::array<uint8_t, 256> phyBuffer;
    size_t phyLen = sdataLayer.wrapBin(phyBuffer, data, len, addr, sig, inst);
    phyLen = frameLayer.wrapBin(phyBuffer, phyLen);
    uart.sendData(phyBuffer.data(), phyLen);
}

void SpinelProtocolManager::sendRequest(char addr, const char *inst, const char *data, size_t len)
{
    if (len > 247)
        return;
    // Wrap payload with ASCII Spinel protocol bytes.
    std::array<uint8_t, 256> phyBuffer;
    size_t phyLen = sdataLayer.wrapAscii(phyBuffer, data, len, static_cast<uint8_t>(addr), inst);
    phyLen = frameLayer.wrapAscii(phyBuffer, phyLen);
    uart.sendData(phyBuffer.data(), phyLen);
}

void SpinelProtocolManager::sendResponse(uint8_t addr, uint8_t ackCode, uint8_t sig,
                                         const uint8_t *data, size_t len, bool isAscii)
{
    // ASCII ACK is passed as a string to reuse the request wrapper.
    if (isAscii)
    {
        char ackStr[2] = {static_cast<char>(ackCode), '\0'};
        sendRequest(static_cast<char>(addr), ackStr, reinterpret_cast<const char *>(data), len);
    }
    else
    {
        sendRequest(addr, ackCode, sig, data, len);
    }
}
