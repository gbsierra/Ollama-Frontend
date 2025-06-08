#include <iostream>
#include <string>
#include <cstdlib>  // For the system() function
#include <functional>
#include <windows.h>
#include <thread>
#include <cstdio>
#include <memory>
#include <stdexcept>

// ModelClient class for interacting with Ollama
class ModelClient {
private:
    std::string model;

public:
    // Constructor to set the model name
    ModelClient(const std::string& modelName) : model(modelName) {}

    // setModel() - Method to set a new model
    void setModel(const std::string& modelName) {
        model = modelName;
    }

    // sendPrompt() - Method to send a prompt and get a response
    std::string sendPrompt(const std::string& prompt) {
        // Check if model is specified
        if (model.empty()) {
            throw std::runtime_error("Model name is not specified");
        }

        // Escape special characters in the prompt
        std::string escapedPrompt;
        for (char c : prompt) {
            if (c == '"' || c == '\\') {
                escapedPrompt += '\\';
            }
            escapedPrompt += c;
        }

        // Construct the command to match working syntax
        std::string command = "ollama run " + model + " \"" + escapedPrompt + "\"";

        // Debug: Print the command being executed
        std::cout << "Executing command: " << command << std::endl;

        // Execute the command
        std::string result = OpenTerminal(command, true);

        // Debug: Print the result
        std::cout << "Command output: " << result << std::endl;

        return result;
    }

    // getModel() - Method to get the current model name
    std::string getModel() const {
        return model;
    }

    // OpenTerminal() - Method to open windows terminal
    static std::string OpenTerminal(const std::string& command, bool wait) {
        static bool consoleAllocated = false;

        // Allocate console if needed
        if (!consoleAllocated) {
            FreeConsole();
            if (AllocConsole()) {
                FILE* dummy;
                if (freopen_s(&dummy, "CONOUT$", "w", stdout) != 0) {
                    DWORD error = GetLastError();
                    return "Failed to redirect stdout, error: " + std::to_string(error);
                }
                if (freopen_s(&dummy, "CONOUT$", "w", stderr) != 0) {
                    DWORD error = GetLastError();
                    return "Failed to redirect stderr, error: " + std::to_string(error);
                }
                if (freopen_s(&dummy, "CONIN$", "r", stdin) != 0) {
                    DWORD error = GetLastError();
                    return "Failed to redirect stdin, error: " + std::to_string(error);
                }
                consoleAllocated = true;

                // Ensure console is visible
                HWND consoleWindow = GetConsoleWindow();
                if (consoleWindow) {
                    ShowWindow(consoleWindow, SW_SHOW);
                    SetForegroundWindow(consoleWindow);
                }

                // Direct console write
                HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
                if (hConsole != INVALID_HANDLE_VALUE) {
                    const char* testMsg = "DEBUG: Console allocated successfully!\n";
                    DWORD written;
                    WriteConsoleA(hConsole, testMsg, strlen(testMsg), &written, nullptr);
                }

            }
            else {
                DWORD error = GetLastError();
                return "Console allocation failed, error: " + std::to_string(error);
            }
        }

        // Test output for every call
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole != INVALID_HANDLE_VALUE) {
            std::string testMsg = "COMMAND: " + command + "\n";
            DWORD written;
            WriteConsoleA(hConsole, testMsg.c_str(), testMsg.length(), &written, nullptr);
        }

        if (command.empty()) {
            return "Empty command";
        }

        // Append "< nul 2>&1" to redirect stdin and capture stdout/stderr
        std::string fullCommand = command + " < nul 2>&1";

        // Output string to hold the raw result
        std::string result;

        // Use _popen to capture output
        std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(fullCommand.c_str(), "r"), _pclose);
        if (!pipe) {
            return "Failed to open pipe.";
        }

        // Read command output into result
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
            result += buffer;
        }

        // Clean ANSI escape codes
        std::string cleanResult;
        bool inEscape = false;
        for (char c : result) {
            if (c == '\033' || c == 27) { // Start of ANSI escape sequence
                inEscape = true;
                continue;
            }
            if (inEscape) {
                if (c == 'm' || c == 'h' || c == 'l' || c == 'K') { // Common ANSI sequence terminators
                    inEscape = false;
                }
                continue;
            }
            if (std::isprint(static_cast<unsigned char>(c)) || c == '\n') {
                cleanResult += c;
            }
        }

        // Test output for raw and clean result
        if (hConsole != INVALID_HANDLE_VALUE) {
            std::string outputMsg = "OUTPUT: " + cleanResult + "\n";
            DWORD written;
            WriteConsoleA(hConsole, outputMsg.c_str(), outputMsg.length(), &written, nullptr);
        }

        // Check _pclose status
        int status = _pclose(pipe.get());
        if (status != 0) {
            return "Command failed with status " + std::to_string(status) + ": " + cleanResult;
        }

        /* Wait for user input
        if (wait) {
            // Get handle to standard output
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD written;

            // Display message using WriteConsoleA
            const char* message = "Press Enter to continue...\n";
            WriteConsoleA(hConsole, message, strlen(message), &written, nullptr);

            // Wait for user input
            std::cin.get();
        } */


        return cleanResult; // Return cleaned output
    }

};
