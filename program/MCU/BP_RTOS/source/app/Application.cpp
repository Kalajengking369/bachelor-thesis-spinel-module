#include "Application.h"
#include "ProtocolConstants.h"
#include "SpinelBuffers.h"
#include "storage/ConfigMapper.h"
#include "fsl_common.h"

Application::Application(DeviceManager &deviceManager, volatile uint8_t &commsErrors, ConfigStorage &storage)
    : devs(deviceManager), handlers(deviceManager, commsErrors), storage(storage) {}

TxBuffer Application::onFrameReceived(const RxBuffer &rawRx, const UnwrappedSData &sdata, bool isAscii)
{
    TxBuffer tx;
    tx.hasResponse = false;

    if (sdata.payload.isEmpty())
        return tx;

    // Resolve Spinel addressing mode before selecting target device.
    uint8_t addr = sdata.addr;
    bool isBroadcast = isAscii ? (addr == '%') : (addr == 0xFF);
    bool isUniversal = isAscii ? (addr == '$') : (addr == 0xFE);
    bool isUnicast = !isBroadcast && !isUniversal;

    Device *devPtr = nullptr;
    if (isUniversal)
    {
        devPtr = devs.getFirstActiveDevice();
    }
    else if (isUnicast)
    {
        devPtr = devs.getDevice(addr);
    }

    if (isUnicast && devPtr == nullptr)
    { // Ignore unicast frames for unknown addresses.
        return tx;
    }

    if (rawRx.flags & (RxFlags::OVERFLOW | RxFlags::SHORT_FRAME))
    {
        if (!isBroadcast && devPtr != nullptr)
        {
            tx.addr = devPtr->spinelAddr;
            tx.ackCode = (isAscii) ? AckAscii::INVALID_DATA : AckBin::INVALID_DATA;
            tx.hasResponse = true;
            tx.len = 0;
        }
        return tx;
    }

    // Checksum error is reported only when checksum checking is enabled for the device.
    if ((rawRx.flags & RxFlags::CRC_ERROR) && !isBroadcast)
    {
        if (devPtr != nullptr && devPtr->checksumEnabled)
        {
            tx.addr = devPtr->spinelAddr;
            tx.ackCode = (isAscii) ? AckAscii::INVALID_DATA : AckBin::INVALID_DATA;
            tx.hasResponse = true;
            tx.len = 0;
            return tx;
        }
    }

    // Broadcast commands are processed by all devices but do not generate a response.
    if (isBroadcast)
    {
        for (Device &dev : devs.getDevices())
        {
            if ((rawRx.flags & RxFlags::CRC_ERROR) && dev.checksumEnabled)
            {
                continue;
            }
            (void)handlers.process(sdata.payload, isAscii, dev, false);
        }
        return tx;
    }

    if (devPtr == nullptr)
    {
        return tx;
    }

    tx = handlers.process(sdata.payload, isAscii, *devPtr, isUnicast);
    tx.hasResponse = true;
    if (tx.hasResponse)
    {
        tx.addr = devPtr->spinelAddr;
    }

    return tx;
}

void Application::onResponseSent(bool needReset)
{
    // Delayed actions are executed only after the response has been sent.
    if (needReset)
    {
        NVIC_SystemReset();
    }
    else
    {
        StorageConfig cfg{};
        exportConfig(devs, cfg);
        storage.save(cfg);
    }
}