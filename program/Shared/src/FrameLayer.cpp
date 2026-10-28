#include "FrameLayer.h"

#include "ProtocolConstants.h"
#include "SpinelUtils.h"

using Protocol::CR;
using Protocol::PRE;

UnwrappedFrame FrameLayer::unwrapFrame(SpinelBufferView frame)
{
    UnwrappedFrame unwrapped;
    if (frame.len < 2)
        return unwrapped;

    unwrapped.isAscii = SpinelUtils::isAsciiFrame(frame.data[1]);

    if (!unwrapped.isAscii)
    {
        if (frame.len < 6)
            return unwrapped; // PRE + FRM + NUM1 + NUM2 + SUM + CR
        unwrapped.frameValid = true;
        unwrapped.payload = {frame.data + 4, frame.len - 6};
        return unwrapped;
    }

    if (frame.len < 3)
        return unwrapped; // PRE + FRM + CR
    unwrapped.frameValid = true;
    unwrapped.payload = {frame.data + 2, frame.len - 3};
    return unwrapped;
}

uint8_t FrameLayer::calcSum(const uint8_t *data, size_t len)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return 0xFF - sum;
}

size_t FrameLayer::wrapBin(std::array<uint8_t, 256> &buf, size_t endIdx)
{
    uint16_t lenField = (endIdx - 4) + 2; // SDATA + SUM + CR

    buf[0] = PRE;
    buf[1] = static_cast<uint8_t>(Protocol::BIN97);
    buf[2] = (lenField >> 8) & 0xFF;
    buf[3] = lenField & 0xFF;

    uint8_t sum = calcSum(buf.data(), endIdx);

    buf[endIdx] = sum;
    endIdx++;

    buf[endIdx] = CR;
    endIdx++;

    return endIdx;
}

size_t FrameLayer::wrapAscii(std::array<uint8_t, 256> &buf, size_t endIdx)
{
    buf[0] = PRE;
    buf[1] = Protocol::ASCII66;
    buf[endIdx] = CR;
    endIdx++;

    return endIdx;
}