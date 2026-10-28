#include "SDataLayer.h"

#include "SpinelUtils.h"

UnwrappedSData SDataLayer::unwrapSData(SpinelBufferView sdata, bool isAscii)
{

    UnwrappedSData unwrapped;
    if (sdata.len < 1)
        return unwrapped;
    if (isAscii)
    {
        // Unwrap ADR
        unwrapped.addr = sdata.data[0];
        unwrapped.addrValid = true;
        if (sdata.len < 2) // Need ADR + INST
            return unwrapped;
        unwrapped.payload = {sdata.data + 1, sdata.len - 1};
        return unwrapped;
    }
    // Unwrap ADR + SIG
    unwrapped.addr = sdata.data[0];
    unwrapped.addrValid = true;
    if (sdata.len < 2)
        return unwrapped;
    unwrapped.sig = sdata.data[1];
    unwrapped.sigValid = true;
    if (sdata.len < 3) // Need ADR + SIG + INST
        return unwrapped;
    unwrapped.payload = {sdata.data + 2, sdata.len - 2};
    return unwrapped;
}

size_t SDataLayer::wrapBin(std::array<uint8_t, 256> &buf, const uint8_t *payload, size_t len,
                           uint8_t addr, uint8_t sig, uint8_t inst)
{
    size_t offset = 4; // PRE + FRM + NUM + NUM
    buf[offset++] = addr;
    buf[offset++] = sig;
    buf[offset++] = inst;

    for (size_t i = 0; i < len; i++)
    {
        buf[offset++] = payload[i];
    }

    return offset;
}

size_t SDataLayer::wrapAscii(std::array<uint8_t, 256> &buf, const char *payload, size_t len,
                             uint8_t addr, const char *inst)
{
    size_t offset = 2; // PRE + FRM

    buf[offset++] = addr;
    if (inst)
    {
        for (size_t i = 0; inst[i] != '\0'; i++)
            buf[offset++] = inst[i];
    }
    for (size_t i = 0; i < len; i++)
        buf[offset++] = payload[i];

    return offset;
}