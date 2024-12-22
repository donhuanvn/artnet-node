#ifndef __MISCELLANEOUS_H__
#define __MISCELLANEOUS_H__

struct cJSON;

class HWStatus
{
public:
    enum Mode
    {
        CONFIG_ONLY,
        CONFIG_AND_RUN_DMX,
    };
    static Mode GetMode();
    static float GetBrightValue();
    static float GetSpeedValue();
};

#endif /* __MISCELLANEOUS_H__ */