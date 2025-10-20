/**
 * @file Interface.h
 * @author ETHAN NG YONG LE (n.ethanyongle@digipen.edu)
 * @brief Abstract interface for all engine systems
 * @date 2025-09-30
 *
 * Defines the base interface that all engine systems must implement.
 * Provides a uniform API for initialization, per-frame updates, and
 * inter-system messaging.
 */
#pragma once

// Forward declaration
class Message;

/**
 * @class InterfaceSystem
 * @brief Abstract base class for all engine systems
 *
 * All systems in the engine (Graphics, Input, Movement, Collision, etc.)
 * must inherit from this interface and implement its pure virtual methods.
 * This ensures consistent lifecycle management and communication across
 * all engine subsystems.
 *
 * System Lifecycle:
 * 1. Construction
 * 2. Initialize() - called once at startup
 * 3. Update(dt) - called every frame
 * 4. SendEngineMessage() - receives messages from other systems
 * 5. Destruction
 *
 * Example implementation:
 * @code
 * class MySystem : public InterfaceSystem {
 * public:
 *     void Initialize() override { // setup }
 *     void Update(float dt) override { // per-frame logic }
 *     void SendEngineMessage(Message* msg) override { // handle messages }
 * };
 * @endcode
 */
class InterfaceSystem
{
public:
    /**
     * @brief Virtual destructor for proper cleanup
     *
     * Ensures derived system destructors are called correctly
     * when deleting through base class pointer.
     */
    virtual ~InterfaceSystem() {}

    /**
     * @brief Initializes the system
     *
     * Called once during engine startup after the system is created.
     * Use this for one-time setup such as loading resources, creating
     * render contexts, or initializing subsystem state.
     *
     * @note Systems are initialized in the order they are added to CoreEngine
     */
    virtual void Initialize() = 0;

    /**
     * @brief Updates the system each frame
     * @param dt Delta time since last frame in seconds
     *
     * Called every frame by the game loop. Implement per-frame logic here
     * such as processing input, updating physics, or rendering graphics.
     *
     * @note Use dt for frame-rate independent behavior: position += velocity * dt
     */
    virtual void Update(float dt) = 0;

    /**
     * @brief Receives messages from other systems
     * @param message Pointer to the message being sent
     *
     * Systems communicate through a message-passing system. Implement this
     * to handle relevant messages such as Quit, WindowResize, or custom events.
     *
     * Common messages:
     * - Status::Quit - Application is shutting down
     *
     * @note Check message->MessageId to determine message type
     */
    virtual void SendEngineMessage(Message* message) = 0;

};