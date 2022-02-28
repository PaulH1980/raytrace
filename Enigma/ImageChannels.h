#pragma once

namespace RayTrace
{
    enum ChannelMask {
        CHANNEL_UNDEFINED = 0x0,
        CHANNEL_RED = 0x01,
        CHANNEL_GREEN = 0x02,
        CHANNEL_BLUE = 0x04,
        CHANNEL_ALPHA = 0x08,
        CHANNEL_RG = CHANNEL_RED | CHANNEL_GREEN,
        CHANNEL_RGB = CHANNEL_RG | CHANNEL_BLUE,
        CHANNEL_RGBA = CHANNEL_RED | CHANNEL_GREEN | CHANNEL_BLUE | CHANNEL_ALPHA
    };

    inline ChannelMask ChannelCountToMask(int _count) {
        switch (_count)
        {
        case 0:
            return CHANNEL_UNDEFINED;
        case 1:
            return CHANNEL_RED;
        case 2:
            return CHANNEL_RG;
        case 3:
            return CHANNEL_RGB;
        case 4:
            return CHANNEL_RGBA;
        default:
            return CHANNEL_UNDEFINED;
        }


    }



    inline int ChannelCount(ChannelMask _in)
    {
        int numChannels = 0;
        if (_in & CHANNEL_RED)
            numChannels++;
        if (_in & CHANNEL_GREEN)
            numChannels++;
        if (_in & CHANNEL_BLUE)
            numChannels++;
        if (_in & CHANNEL_ALPHA)
            numChannels++;
        return numChannels;
    }
}