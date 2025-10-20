/**
 * @file Message.h
 * @author ETHAN NG YONG LE (n.ethanyongle@digipen.edu)
 * @brief Simple message system for inter-system communication
 * @date 2025-09-30
 *
 * Provides a lightweight message-passing mechanism for systems to communicate
 * without tight coupling. Systems can broadcast messages through the CoreEngine,
 * which distributes them to all registered systems.
 */
#pragma once

 /**
  * @enum Status
  * @brief Message type identifiers
  *
  * Defines the types of messages that can be sent between systems.
  * Each message type represents a different event or command.
  */
enum Status
{
    Success,
    Error,
    Quit,
};

/**
 * @class Message
 * @brief Container for inter-system messages
 *
 * Simple message structure that carries a message ID to identify
 * the message type. Systems can check the MessageId to determine
 * how to respond to the message.
 *
 * Usage example:
 * @code
 * // Sending a quit message
 * Message quitMsg(Status::Quit);
 * engine.BroadcastMessage(&quitMsg);
 *
 * // Receiving a message in a system
 * void MySystem::SendEngineMessage(Message* msg) {
 *     if (msg->MessageId == Status::Quit) {
 *         // Handle quit
 *     }
 * }
 * @endcode
 *
 * @note This is a basic implementation. Future extensions could include
 *       message data payloads, priority levels, or sender identification.
 */
class Message
{
public:
    Status MessageId; //Type identifier for message

    /**
     * @brief Constructs a message with the specified ID
     * @param id The message type/status
     */
    Message(Status id) : MessageId(id) {}
};