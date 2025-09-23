#pragma once

// Message IDs - these identify what type of message it is
enum Status
{
    Success,
    Error,
    Quit,
    // We'll add more message types later as needed
};

class Message
{
public:
    Status MessageId;

    // Constructor to make creating messages easier
    Message(Status id) : MessageId(id) {}
};