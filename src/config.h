#ifndef config_h
#define config_h

#define DEVICE_CONF_ARRAY_LENGHT 50

class config_t
{
  public:
    config_t() {};
    int confirmation;
    char broker[DEVICE_CONF_ARRAY_LENGHT];
    char topicHumidity[DEVICE_CONF_ARRAY_LENGHT];
    char wifiSsid[DEVICE_CONF_ARRAY_LENGHT];
    char wifiPass[DEVICE_CONF_ARRAY_LENGHT];
    char ioUser[DEVICE_CONF_ARRAY_LENGHT];
    char ioKey[DEVICE_CONF_ARRAY_LENGHT];
};

#endif
