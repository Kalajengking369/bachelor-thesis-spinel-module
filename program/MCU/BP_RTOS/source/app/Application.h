#pragma once
#include "ISpinelApplication.h"
#include "InstructionHandlers.h"
#include "DeviceManager.h"
#include "storage/ConfigStorage.h"

class Application : public ISpinelApplication
{
private:
	DeviceManager &devs;
	ConfigStorage &storage;
	InstructionHandlers handlers;
	TxBuffer onFrameReceived(const RxBuffer &rawRx, const UnwrappedSData &sdata, bool isAscii) override;
	void onResponseSent(bool needReset) override;

public:
	Application(DeviceManager &deviceManager, volatile uint8_t &commsErrors, ConfigStorage &storage);
};