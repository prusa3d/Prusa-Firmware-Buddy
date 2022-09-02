#pragma once

class IConfigurationStoreBackend {
public:
    template <class T>
    void set(const char *key, const T &data);
};
