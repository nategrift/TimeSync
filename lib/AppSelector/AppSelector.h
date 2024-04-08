#ifndef APP_SELECTOR_H
#define APP_SELECTOR_H

#include "AppManager.h"
#include "ListComponent.h"
#include "IApp.h"

class AppSelector : public IApp {
private:
    int listComponentId;

public:
    AppSelector(AppManager& manager);
    ~AppSelector() override;

    void launch() override;
    void close() override;
    void backgroundActivity() override;


};

#endif // APP_SELECTOR_H
