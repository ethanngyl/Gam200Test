#pragma once

// Forward declaration - we'll define Message later
class Message;

class InterfaceSystem
{
public:
    virtual ~InterfaceSystem() {}
    virtual void Initialize() = 0;
    virtual void Update(float dt) = 0;
    virtual void SendMessage(Message* message) = 0;
};