#pragma once
#include "Interface.h"

class WindowSystem : public InterfaceSystem
{
public:
    WindowSystem();
    virtual ~WindowSystem();

    // ISystem interface
    virtual void Initialize() override;
    virtual void Update(float dt) override;
    virtual void SendMessage(Message* message) override;

private:
    bool WindowOpen;
};