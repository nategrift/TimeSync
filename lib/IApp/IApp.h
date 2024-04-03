#ifndef IAPP_H
#define IAPP_H

class AppManager;

class IApp {
protected: 
    AppManager& appManager;

    IApp(AppManager& manager) : appManager(manager) {}

public:
    virtual ~IApp() = default;
    virtual void launch() = 0;
    virtual void close() = 0;
    virtual void backgroundActivity() = 0;
};

#endif // IAPP_H