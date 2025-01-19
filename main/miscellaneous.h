#ifndef __MISCELLANEOUS_H__
#define __MISCELLANEOUS_H__

#include <stdio.h>
#include <memory>

#ifndef PROJECT_DMX_MESSAGE_BUFFER_SIZE
#define PROJECT_DMX_MESSAGE_BUFFER_SIZE 530
#endif

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

class DMX512Message
{
    char *pData;

public:
    DMX512Message()
    {
        pData = new char[PROJECT_DMX_MESSAGE_BUFFER_SIZE];
    }
    DMX512Message(char * pBuffer)
    {
        pData = pBuffer;
    }
    ~DMX512Message()
    {
        delete[] pData;
    }
    int32_t GetUniverse();
    char * GetBuffer() { return pData; }
};

#endif /* __MISCELLANEOUS_H__ */