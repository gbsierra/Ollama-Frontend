#include "imgui.h"
#include "App.h"
#include "ModelClient.cpp"

std::vector<std::string> get_ollama_model_names() {
    std::vector<std::string> model_names;

    // Temporary file to store ollama list output
    const char* temp_file = "ollama_list_output.txt";

    // Run ollama list and redirect output to a file
    std::string command = "ollama list > " + std::string(temp_file);
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Error: Failed to run ollama list command" << std::endl;
        return model_names;
    }

    // Read the output from the temporary file
    std::ifstream file(temp_file);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open temporary file" << std::endl;
        return model_names;
    }

    // Parse the file line by line
    std::string line;
    bool first_line = true; // Skip the header line
    while (std::getline(file, line)) { // Use std::ifstream with std::getline
        if (first_line) {
            first_line = false; // Skip the table header
            continue;
        }
        // Extract the first column (model name)
        std::istringstream ss(line); // Use std::istringstream for parsing the line
        std::string model_name;
        ss >> model_name; // Read the first word (model name)
        if (!model_name.empty()) {
            model_names.push_back(model_name);
        }
    }

    file.close();

    // Clean up the temporary file
    std::remove(temp_file);

    return model_names;
}

std::string get_ollama_model_info(const std::string& model_name) {
    std::string result;

    // Temporary file to store ollama show output
    const char* temp_file = "ollama_show_output.txt";

    // Run ollama show with the model name and redirect output to a file
    std::string command = "ollama show " + model_name + " > " + std::string(temp_file);
    int status = std::system(command.c_str());
    if (status != 0) {
        std::cerr << "Error: Failed to run ollama show command for model " << model_name << std::endl;
        return "Failed to retrieve model info.";
    }

    // Read the output from the temporary file
    std::ifstream file(temp_file);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open temporary file" << std::endl;
        return "Failed to open output file.";
    }

    // Parse the file to extract the Model section
    std::string line;
    std::string model_section;
    bool in_model_section = false;
    while (std::getline(file, line)) {
        // Check for the start of the Model section
        if (line.find("Model") != std::string::npos) {
            in_model_section = true;
            // Extract content after "Model"
            size_t start = line.find("Model") + 6;
            if (start < line.length()) {
                model_section = line.substr(start);
                // Trim leading/trailing whitespace
                while (!model_section.empty() && std::isspace(model_section.front())) {
                    model_section.erase(model_section.begin());
                }
                while (!model_section.empty() && std::isspace(model_section.back())) {
                    model_section.pop_back();
                }
            }
            continue;
        }
        // Collect lines in the Model section until a new section or empty line
        if (in_model_section) {
            if (line.empty() || line.find("Capabilities") != std::string::npos) {
                in_model_section = false; // Stop at empty line or new section
                break;
            }
            // Trim whitespace from the line
            std::string trimmed_line = line;
            while (!trimmed_line.empty() && std::isspace(trimmed_line.front())) {
                trimmed_line.erase(trimmed_line.begin());
            }
            while (!trimmed_line.empty() && std::isspace(trimmed_line.back())) {
                trimmed_line.pop_back();
            }
            if (!trimmed_line.empty()) {
                model_section += "\n" + trimmed_line;
            }
        }
    }

    file.close();

    // Clean up the temporary file
    std::remove(temp_file);

    // Return the model section or an error message if not found
    return model_section.empty() ? "No model info found for " + model_name + "." : model_section;
}

// App Namespace for imgui implementation
namespace App {

    // Declarations
    static std::vector<std::string> model_names = get_ollama_model_names(); // added model names
    static int selected = 0;                                                // selected model index
    ModelClient client(model_names[selected]);                              // inital client
    std::vector<std::string> outputVector;                                  // holds outputs
    std::vector<std::string> inputVector;                                   // holds inputs
    std::string model_info = get_ollama_model_info(model_names[selected]);  // holds current model info
    bool showConsole = false;                                               // show console with output?

    // Renders Main Header - called by RenderApplicationWindow()
    void RenderApplicationHeader() {

        // Set the header position at the top of the screen
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 40));
        ImGui::SetNextWindowBgAlpha(0.9f);
        //ImGui::SetNextWindowFocus();
        ImGui::Begin("HeaderWindow", NULL, ImGuiWindowFlags_NoDecoration );

        // Application name
        ImGui::SetWindowFontScale(1.4f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
        ImGui::Text("  Model Application");

        // minimize and close buttons, aligned to the right
        ImGui::SameLine(ImGui::GetWindowWidth() - 65);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
        if (ImGui::Button("-")) {
            HWND activeWindow = GetForegroundWindow(); // Get the currently active window
            PostMessage(activeWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0); // Minimize the active window

        }
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
        if (ImGui::Button("x")) {
            PostQuitMessage(0); // Close the application
        }

        ImGui::End();
    }
    
    // Renders Main Window
    void RenderApplicationWindow() {

        //round window, set position
        ImGui::GetStyle().WindowRounding = 0.0f;
        ImGui::SetNextWindowPos(ImVec2(0.0f, 40.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(800.0f, 620.0f), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
        ImGui::Begin("Language Model Generator", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);        

        RenderApplicationHeader();

        ImGui::SetCursorPosX(10);
        static char new_model_name[64] = "";

        // "Select Model" dropdown
        if (client.running) {
            ImGui::BeginDisabled();
        }
        ImGui::SetNextItemWidth(465.0f);
        if (ImGui::BeginCombo("Select Model", model_names.empty() ? "No Models" : model_names[selected].c_str())) {
            
            // list of models
            for (int i = 0; i < model_names.size(); i++) {
                bool is_selected = (selected == i);
                
                if (ImGui::Selectable(model_names[i].c_str(), is_selected)) {
                    selected = i;
                    client.setModel(model_names[selected]); // set model
                    model_info = get_ollama_model_info(model_names[selected]); // retrieve model info
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            // input field 'New Model Name'
            ImGui::Separator();
            ImGui::InputText("New Model Name", new_model_name, IM_ARRAYSIZE(new_model_name));
            ImGui::SameLine();
            
            // help '?' button
            if (ImGui::Button("?")) {
                ImGui::OpenPopup("HelpTooltipPopup"); // Open a popup to act as a tooltip
            }
            if (ImGui::BeginPopup("HelpTooltipPopup")) {
                ImGui::Text("For a complete list of model names, \nvisit the link below:");
                if (ImGui::Selectable("Browse Models")) {
                    // Platform-specific command to open URL
                    #ifdef _WIN32
                    std::system("start https://ollama.com/library");
                    #elif __APPLE__
                    std::system("open https://ollama.com/library");
                    #else
                    std::system("xdg-open https://ollama.com/library");
                    #endif
                }
                ImGui::EndPopup();
            }
            
            // add model button
            if (ImGui::Button("Add")) {
                if (strlen(new_model_name) > 0) {
                    model_names.push_back(std::string(new_model_name));
                    selected = model_names.size() - 1; // Select the new model
                    client.setModel(model_names[selected]);
                    memset(new_model_name, 0, sizeof(new_model_name)); // Clear input field
                }
            }

            ImGui::EndCombo();
        }
        if (client.running) {
            ImGui::EndDisabled();
        }

        // model info '?' button
        ImGui::SameLine();
        if (ImGui::Button("?")) {
            ImGui::OpenPopup("ModelInfoPopup");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::SeparatorText("Model Information:");
            ImGui::EndTooltip();
        }
        if (ImGui::BeginPopup("ModelInfoPopup")) {
            ImGui::SeparatorText(model_info.c_str()); // Display the 'Model' section
            ImGui::NewLine();
            
            ImGui::EndPopup();
        }

        // 'show console' toggle button
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 55);
        if (client.running) {
            ImGui::BeginDisabled();
        }
        ImGui::Checkbox("Show Console", &showConsole);
        if (client.running) {
            ImGui::EndDisabled();
        }
        ImGui::Separator();

        // vertical seperator
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddLine(ImVec2(622, 80), ImVec2(622, 650), IM_COL32(255, 255, 255, 120), 0.4f);

        // saved list
        ImGui::SetCursorPos(ImVec2(626, 44));
        ImGui::Text("Saved Chat Histories:");
        ImGui::SetCursorPos(ImVec2(626, 63));
        ImGui::BeginChild("ChatHistoryList", ImVec2(165, 525), true);
        for (int fileNumber = 1; fileNumber <= 60; ++fileNumber) { // temp limit to 60 history. (will soon have filenames given by ai, theres limits without filesystem from C++17)
            std::string filePath = "chat_history/chat_history_" + std::to_string(fileNumber) + ".txt";
            if (!std::ifstream(filePath).good()) {
                continue;
            }
            std::string buttonLabel = "chat_history_" + std::to_string(fileNumber);
            
            //is running? (begin)
            if (client.running) {
                ImGui::BeginDisabled();
            }

            // load chat
            if (ImGui::Selectable(buttonLabel.c_str())) {
                std::ifstream inFile(filePath);
                if (inFile.is_open()) {
                    std::string line;
                    std::string currentMessage;
                    bool isPrompt = false;

                    inputVector.clear();
                    outputVector.clear();

                    // save chat file
                    while (std::getline(inFile, line)) {
                        if (line.empty()) continue;

                        if (line.find("User Prompt: ") == 0) {
                            // Save previous message before resetting
                            if (!currentMessage.empty()) {
                                (isPrompt ? inputVector : outputVector).push_back(currentMessage);
                            }
                            currentMessage = line.substr(12);
                            isPrompt = true;
                        }
                        else if (line.find("Response: ") == 0) {
                            // Save previous message before resetting
                            if (!currentMessage.empty()) {
                                (isPrompt ? inputVector : outputVector).push_back(currentMessage);
                            }
                            currentMessage = line.substr(10);
                            isPrompt = false;
                        }
                        else {
                            currentMessage += (currentMessage.empty() ? "" : "\n") + line;
                        }
                    }
                    // store last msg
                    if (!currentMessage.empty()) {
                        (isPrompt ? inputVector : outputVector).push_back(currentMessage);
                    }

                    inFile.close();
                }
            }
            // right clicked?
            if (ImGui::BeginPopupContextItem()) {
                ImGui::Text("Are you sure you want\nto delete this chat?");
                if (ImGui::Button("Yes")) {
                    if(std::ifstream(filePath).good()) std::remove(filePath.c_str()); // deletion
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("No")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }


            //is running? (end)
            if (client.running) {
                ImGui::EndDisabled();
            }
        }
        ImGui::EndChild();

        // end
        ImGui::End();
        ImGui::PopStyleColor();
    }
    
    // Renders Question Input Window
    void RenderQuestionInputWindow() {

        // set window size, begin, and set pos
        ImGui::SetNextWindowSize(ImVec2(620, 120));
        ImGui::SetNextWindowPos(ImVec2(0, 552));
        ImGui::Begin("Question Input Window", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);

        // Static string to keep input throughout current frame
        static std::string inputText;

        auto InputTextWithResize = [](const char* label, const char* hint, std::string& str) -> bool {
            // Resize callback
            auto callback = [](ImGuiInputTextCallbackData* data) -> int {
                if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                    auto* str = static_cast<std::string*>(data->UserData);
                    str->resize(data->BufTextLen);
                    data->Buf = &(*str)[0]; // FIXED: Writable buffer
                }
                return 0;
                };

            // if buffer has room
            if (str.capacity() == 0) str.reserve(256);

            ImVec2 cursorPos = ImGui::GetCursorScreenPos();

            // input box
            bool inputChanged = ImGui::InputTextMultiline(label, &str[0], str.capacity() + 1, ImVec2(605, 60),
                ImGuiInputTextFlags_CallbackResize, callback, &str);

            // add placeholder text
            if (str.empty()) {
                ImVec2 textPos = ImVec2(cursorPos.x + 5, cursorPos.y + 5); // Corrected addition syntax
                ImGui::GetWindowDrawList()->AddText(textPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), hint);
            }

            return inputChanged;
            };

        // Render input field with increased height
        InputTextWithResize("##YourQuestion", "Enter your message here...", inputText);
        
        // dynamically update output if running
            if (!outputVector.empty() && client.running) {
                outputVector.back() = client.getOutput();
            }
        
        //is running? (begin)
        if (client.running) {
            ImGui::BeginDisabled();
        }

        // Submit button
        if (ImGui::Button("Submit") || (ImGui::IsKeyPressed(ImGuiKey_Enter))) {
            std::string prompt = inputText;  // copy input to avoid lifetime issues

            // Replace all newline characters with spaces
            std::replace(prompt.begin(), prompt.end(), '\n', ' ');

            std::thread([prompt, clientPtr = &client]() {
                std::string result = clientPtr->sendPrompt(prompt, showConsole);
                }).detach();

            outputVector.push_back(client.getOutput());
            //client.clearOutput();
            inputVector.push_back(inputText);
            ImGui::SetKeyboardFocusHere();
            inputText.clear(); // clear input after sending
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::SeparatorText(("Send Question to " + client.getModel()).c_str());
            ImGui::EndTooltip();
        }

        // save button
        ImGui::SetCursorPos(ImVec2(488, 76));
        if (ImGui::Button("Save")) {
            #ifdef _WIN32
                 _mkdir("chat_history");
            #else
                 mkdir("chat_history", 0755);
            #endif
            int fileNumber = 1;
            std::string filePath;
            do {
                filePath = "chat_history/chat_history_" + std::to_string(fileNumber) + ".txt";
                fileNumber++;
            } while (std::ifstream(filePath).good());
            std::ofstream outFile(filePath);
            if (outFile.is_open()) {
                size_t maxMessages = inputVector.size() >= outputVector.size() ? inputVector.size() : outputVector.size();
                for (size_t i = 0; i < maxMessages; ++i) {
                    if (i < inputVector.size() && !inputVector[i].empty()) {
                        outFile << "User Prompt: " << inputVector[i] << "\n\n";
                    }
                    if (i < outputVector.size() && !outputVector[i].empty()) {
                        outFile << "Response: " << outputVector[i] << "\n\n";
                    }
                }
                outFile.close();
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::SeparatorText("Save current chat to a file!");
            ImGui::EndTooltip();
        }

        //is running? (end)
        if (client.running) {
            ImGui::EndDisabled();
        }

        // new button
        ImGui::SetCursorPos(ImVec2(534, 76));
        if (ImGui::Button("New Chat")) {
            inputVector.clear();
            outputVector.clear();
            client.TerminateOllamaTasks(showConsole);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::SeparatorText("Create new chat!");
            ImGui::EndTooltip();
        }

        ImGui::End();
    }

    // Renders Question Output Window
    void RenderQuestionOutputWindow() {
        // set window size, begin, set pos, and round window
        ImGui::GetStyle().WindowRounding = 8.0f;
        ImGui::SetNextWindowSize(ImVec2(616, 475));
        ImGui::SetNextWindowPos(ImVec2(3, 80));
        ImGui::Begin("Question Out Window", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        // scrollable region
        ImGui::BeginChild("ChatRegion", ImVec2(0, ImGui::GetWindowHeight() - 20), true);

        float padding = 5.0f;
        ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - 2 * padding - ImGui::GetStyle().ScrollbarSize);

        // Alternate messages (inputVector/outputVector)
        size_t maxMessages = inputVector.size() >= outputVector.size() ? inputVector.size() : outputVector.size();
        for (size_t i = 0; i < maxMessages; ++i) {
            // Display user prompt if available (right-aligned)
            if (i < inputVector.size() && !inputVector[i].empty()) {
                // Calculate text size
                ImVec2 textSize = ImGui::CalcTextSize(inputVector[i].c_str(), nullptr, false, ImGui::GetWindowWidth() - 2 * padding);

                // Move cursor to the right side for text
                float windowWidth = ImGui::GetWindowWidth();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
                ImGui::SetCursorPosX(windowWidth - textSize.x - padding - 10.0f - ImGui::GetStyle().ScrollbarSize);

                ImVec2 textStart = ImGui::GetCursorScreenPos();

                // Draw bubble for prompt (right-aligned)
                ImVec2 bubbleMin = ImVec2(textStart.x - padding, textStart.y - padding);
                ImVec2 bubbleMax = ImVec2(textStart.x + textSize.x + padding, textStart.y + textSize.y + padding);
                ImGui::GetWindowDrawList()->AddRectFilled(bubbleMin, bubbleMax, IM_COL32(80, 140, 255, 255), 10.0f);

                // Draw prompt text (right-aligned)
                ImGui::TextWrapped("%s", inputVector[i].c_str());
                ImGui::Spacing();
            }

            // Display response if available (left-aligned)
            if (i < outputVector.size() && !outputVector[i].empty()) {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
                   
                // response text
                ImGui::TextWrapped("%s", outputVector[i].c_str());
                ImGui::Spacing();
            }
        }

        ImGui::PopTextWrapPos();

        // Auto-scroll to the bottom for new messages
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }


    // Main Render Function for UI
    void RenderUI() {
        RenderApplicationWindow();

        RenderQuestionInputWindow();
        RenderQuestionOutputWindow();
    }

}