#pragma once

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <expected>
#include <mutex>
#include <functional>
#include <unordered_map>
#include "../pdcurses/include/curses.h"
#include "GameEngine.h"
#include "CommandLineEditor.h"

// Define error types for window resizing
enum class ResizeError {
    TERMINAL_TOO_SMALL
};

// Define error types for initialization
enum class InitError {
    NCURSES_INIT_FAILED,
    COLOR_SUPPORT_MISSING,
    CANNOT_CHANGE_COLOR,
    WINDOW_SETUP_FAILED,
    TERMINAL_TOO_SMALL
};

/**
 * Signal handler class that uses dependency injection instead of global state.
 * This class manages signal handlers and routes them to the appropriate target.
 */
class SignalHandler {
public:
    // Function type for signal callbacks
    using SignalCallback = std::function<void()>;

    // Register a callback for a specific signal
    static void registerHandler(int signal, SignalCallback callback);
    
    // Remove a registered callback
    static void unregisterHandler(int signal);
    
    // Process a signal by calling the appropriate callback
    static void handleSignal(int signal);

private:
    // Map of signal numbers to callbacks
    static std::unordered_map<int, SignalCallback> s_signalCallbacks;
    
    // Mutex to protect the callbacks map during registration/unregistration
    static std::mutex s_signalMutex;
};

/**
 * ConsoleUI class manages the user interface of the application using ncurses.
 * It handles input/output, window management, and coordinates with the game engine.
 */
class ConsoleUI {
private:
    // Define the signal callback type
    using SignalCallback = SignalHandler::SignalCallback;
    
    // Window management with RAII unique pointers
    std::unique_ptr<WINDOW, decltype(&delwin)> m_outputWin{nullptr, delwin};
    std::unique_ptr<WINDOW, decltype(&delwin)> m_outputBorderWin{nullptr, delwin};
    std::unique_ptr<WINDOW, decltype(&delwin)> m_inputWin{nullptr, delwin};
    std::unique_ptr<WINDOW, decltype(&delwin)> m_inputBorderWin{nullptr, delwin};

    // Terminal dimensions
    int m_termHeight;
    int m_termWidth;
    int m_outputHeight = 20;
    int m_inputHeight = 3;
    const int m_minHeight = 10;
    const int m_minWidth = 40;

    // Game engine instance
    GameEngine m_game;
    
    // Command line editor that handles input and history
    std::unique_ptr<CommandLineEditor> m_lineEditor;

    // Output buffer for game messages
    std::vector<std::string> m_outputBuffer;
    int m_scrollOffset = 0;

    // Thread-safe flag indicating if the UI is running
    std::atomic<bool> m_isRunning{false};
    
    // Result of window setup - may contain error if the terminal is too small
    std::expected<void, ResizeError> m_resizeStatus;
    
    // Mutex for thread-safe access to the output buffer
    std::mutex m_outputMutex;
    
    // Signal handler callbacks
    SignalCallback m_interruptCallback;
    SignalCallback m_terminateCallback;

    // UI methods
    void handleInput();
    void handleResize();
    void drawLayout();
    void drawOutputWindow();
    void drawInputWindow();
    void addOutputMessage(const std::string& message);
    void processCommand(const std::string& command);
    void cleanupNcurses();
    bool initializeNcurses();
    std::expected<void, ResizeError> setupWindows(int height, int width);
    
    // Initialize signal handlers
    void setupSignalHandlers();
    
    // Clean up signal handlers
    void cleanupSignalHandlers();

public:
    // Constructor with terminal dimensions and player name
    ConsoleUI(int termHeight, int termWidth, const std::string& playerName = "Kieran");
    
    // Destructor ensures ncurses is properly cleaned up
    ~ConsoleUI();

    // Prevent copying (ncurses windows can't be copied)
    ConsoleUI(const ConsoleUI&) = delete;
    ConsoleUI& operator=(const ConsoleUI&) = delete;

    // Allow moving to transfer ownership
    ConsoleUI(ConsoleUI&& other) noexcept;
    ConsoleUI& operator=(ConsoleUI&& other) noexcept;

    // Factory method to create and initialize the UI
    static std::expected<ConsoleUI, InitError> create();
    
    // Main UI loop
    void run();
    
    // Stop the UI loop
    void stop();
    
    // Process a game command
    void handleGameCommand(const std::string& cmd, const std::string& args);
}; 