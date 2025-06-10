#include <iostream>
#include <string>
#include <cstdlib>  // For the system() function
#include <functional>
#include <windows.h>
#include <thread>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <sstream>


// ModelClient class for interacting with Ollama
class ModelClient {
private:
    std::string model;
    std::string output;

public:
    // Constructor
    ModelClient(const std::string& modelName){
        model = modelName;
    }

    // setModel() - Mutator to set a new model
    void setModel(const std::string& modelName) {
        model = modelName;
    }

    // getModel() - Accessor to get the current model name
    std::string getModel() const {
        return model;
    }
    
    // getOutput() - Accessor to get output
    std::string getOutput() const {
        return output;
    }

    // setOutput() - Mutator for output
    void setOutput(const std::string& output) {
        this->output = output;
    }
    
    // clearOutput() - clears output
    void clearOutput() {
        output.clear();
    }

    // sendPrompt() - Method to send a prompt and get a response
    std::string sendPrompt(const std::string& prompt, bool showConsole = true) {
        // Check if has model
        if (model.empty()) {
            throw std::runtime_error("Model name is not specified");
        }

        // Escape special characters in prompt
        std::string escapedPrompt;
        for (char c : prompt) {
            if (c == '"' || c == '\\') {
                escapedPrompt += '\\';
            }
            escapedPrompt += c;
        }

        // Construct command
        std::string command = "ollama run " + model + " \"" + escapedPrompt + "\"";

        // Execute command (recieves response dynamically)
        std::string result = OpenTerminal(command, true, showConsole);

        return result;
    }

    // removeAnsiCodes() - helper func
    std::string removeAnsiCodes(const std::string& text, std::string& buffer) {
        std::string result;
        bool in_escape = !buffer.empty(); // Continue from previous escape state?

        buffer += text; // Append new text to buffer to handle partial sequences


        for (size_t i = 0; i < buffer.length(); ++i) {
            if (buffer[i] == '\033' || buffer[i] == 27) { // Start of ANSI escape sequence
                in_escape = true;
                continue;
            }
            if (in_escape) {
                // if a common ANSI sequence terminator
                if (buffer[i] == 'm' || buffer[i] == 'h' || buffer[i] == 'l' || buffer[i] == 'K') {
                    in_escape = false;
                }
                continue;
            }
            if (std::isprint(static_cast<unsigned char>(buffer[i])) || buffer[i] == '\n') {
                result += buffer[i];
            }
        }

        // If still in an escape sequence, keep the partial sequence in buffer
        if (in_escape) {
            size_t last_complete = buffer.find_last_of("mhlK");
            if (last_complete != std::string::npos && last_complete + 1 < buffer.length()) {
                buffer = buffer.substr(last_complete + 1);
            }
            else {
                buffer.clear();
            }
        }
        else {
            buffer.clear();
        }

        return result;
    }

    // OpenTerminal() - Open terminal and execute command
    std::string OpenTerminal(const std::string& command, bool wait, bool showConsole = true) {
        static bool consoleAllocated = false;
        static std::string ansiBuffer; // Persistent buffer for ANSI sequences

        // showing console
        if (showConsole) {
            // Always attempt to allocate or ensure console is available
            if (!consoleAllocated) {
                FreeConsole(); // Ensure clean state
                if (AllocConsole()) {
                    FILE* dummy;
                    freopen_s(&dummy, "CONOUT$", "w", stdout);
                    freopen_s(&dummy, "CONOUT$", "w", stderr);
                    freopen_s(&dummy, "CONIN$", "r", stdin);
                    consoleAllocated = true;

                    HWND consoleWindow = GetConsoleWindow();
                    if (consoleWindow) {
                        ShowWindow(consoleWindow, SW_SHOW);
                        SetForegroundWindow(consoleWindow);
                    }

                    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
                    if (hConsole != INVALID_HANDLE_VALUE) {
                        const char* testMsg = "DEBUG: Console allocated successfully!\n\n";
                        DWORD written;
                        WriteConsoleA(hConsole, testMsg, strlen(testMsg), &written, nullptr);
                    }
                }
                else {
                    return "Console allocation failed!";
                }
            }
            else {
                // Ensure console is visible if already allocated
                HWND consoleWindow = GetConsoleWindow();
                if (consoleWindow) {
                    ShowWindow(consoleWindow, SW_SHOW);
                    SetForegroundWindow(consoleWindow);
                }
            }
        }
        // not showing console
        else {
            // free & hide console
            if (!consoleAllocated) {
                AllocConsole();
            }
            HWND consoleWindow = GetConsoleWindow();
            ShowWindow(consoleWindow, SW_HIDE);    

        }

        // Check if given a command
        if (command.empty()) {
            return "Empty command!";
        }

        // Execute command and capture output
        std::string fullCommand = "cmd.exe /C \"" + command + " < nul 2>&1\"";
        std::string result;

        std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(fullCommand.c_str(), "r"), _pclose);
        if (!pipe) {
            return "Failed to open pipe.";
        }

        char buffer[128];
        // handle stream in chunks
        while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
            std::string chunk(buffer);
            std::string cleanChunk = removeAnsiCodes(chunk, ansiBuffer); // Pass persistent buffer
            result += cleanChunk;
            setOutput(result); // Set output for imgui to dynamically update
            if (showConsole && consoleAllocated) { // write to allocated console
                HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
                if (hConsole != INVALID_HANDLE_VALUE) {
                    DWORD written;
                    WriteConsoleA(hConsole, cleanChunk.c_str(), (DWORD)cleanChunk.length(), &written, nullptr);
                }
            }
        }

        int status = _pclose(pipe.get());
        if (status != 0) {
            return "Command failed with status " + std::to_string(status) + ": " + result;
        }

        return result;
    }


};
