#include "miscellaneous.h"
#include "models/settings.h"

HWStatus::Mode HWStatus::GetMode()
{
    // Todo: Read status from corresponding GPIOs to decide which mode is set up.
    return CONFIG_AND_RUN_DMX;
}

float HWStatus::GetBrightValue()
{
    // Todo
    return -1.0;
}

float HWStatus::GetSpeedValue()
{
    // Todo
    return -1.0;
}

int32_t DMX512Message::GetUniverse()
{
    return pData[15] << 8 | pData[14];
}
