#pragma once

// Message IDs - these identify what type of message it is
enum Mid
{
    Quit,
    // We'll add more message types later as needed
};

class Message
{
public:
    Mid MessageId;

    // Constructor to make creating messages easier
    Message(Mid id) : MessageId(id) {}
};