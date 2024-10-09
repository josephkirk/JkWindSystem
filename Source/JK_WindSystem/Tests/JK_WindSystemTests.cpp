#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FJK_WindSystemTestsModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FJK_WindSystemTestsModule, JK_WindSystemTests)
