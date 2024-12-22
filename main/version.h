#ifndef __ARTNET_NODE_VERSION_H__
#define __ARTNET_NODE_VERSION_H__

struct cJSON;

class FWVersion
{
public:
    static constexpr const char * version = "1.0.0";
    static constexpr const char * buildTime = "2024-06-20 16:06:00.176788";
    static constexpr const char * commitId = "1e92a576689c77f300efdb80705f536923a2d2fc";
    static void log();
    static cJSON * toJson();
};

#endif /* __ARTNET_NODE_VERSION_H__ */