#ifndef IAPP_H
#define IAPP_H

#include <string>

class IApp {
protected:
    std::string appName;
public:
    IApp(const std::string& name) : appName(name) {}
    virtual ~IApp() = default;
    virtual void launch() = 0;
    virtual void close() = 0;
    virtual void backgroundActivity() = 0;
    std::string getAppName() const { return appName; }
};

#endif // IAPP_H
