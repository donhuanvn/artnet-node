#include "miscellaneous.h"
#include "models/settings.h"
#include "driver/gpio.h"
#include "config.h"


HWStatus::Mode HWStatus::GetMode()
{
    int32_t s32Level = gpio_get_level(static_cast<gpio_num_t>(PROJECT_GPIO_INPUT_MODE_SELECT_0));
    switch (s32Level)
    {
    case 0:
        return CONFIG_ONLY;
    default:
        return CONFIG_AND_RUN_DMX;
    }
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
