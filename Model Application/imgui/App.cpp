#include "App.h"

#include "ModelClient.cpp"

#include "imgui.h"
#include <string>
#include <memory>
#include <iostream>
#include <cstdlib>
#include <Windows.h>

// App Namespace for imgui implementation
namespace App {

    // Declarations
    bool show_question_input = false;
    bool show_question_output = false;

    ModelClient client("llama3.2");
    std::string model_name = client.getModel();
    std::string output = "";


    // Renders Main Header - called by RenderApplicationWindow()
    void RenderApplicationHeader() {

        // Set the header position at the top of the screen
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 50));
        ImGui::SetNextWindowBgAlpha(0.9f);
        //ImGui::SetNextWindowFocus();
        ImGui::Begin("HeaderWindow", NULL, ImGuiWindowFlags_NoDecoration );

        // Application name
        ImGui::SetWindowFontScale(1.4f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        ImGui::Text("  Model Application");

        // Add minimize and close buttons, aligned to the right
        ImGui::SameLine(ImGui::GetWindowWidth() - 65);
        if (ImGui::Button("-")) {
            HWND activeWindow = GetForegroundWindow(); // Get the currently active window
            PostMessage(activeWindow, WM_SYSCOMMAND, SC_MINIMIZE, 0); // Minimize the active window

        }
        ImGui::SameLine();
        if (ImGui::Button("x")) {
            PostQuitMessage(0); // Close the application
        }

        ImGui::End();
    }
    
    // Renders Main Window
    void RenderApplicationWindow() {
        ImGui::GetStyle().WindowRounding = 0.0f;
        ImGui::SetNextWindowPos(ImVec2(0.0f, 50.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400.0f, 350.0f), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
        ImGui::Begin("Language Model Generator", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);        

        RenderApplicationHeader();

        ImGui::Separator();
        ImGui::SetCursorPosX(10);
        ImGui::Text("Using : %s ", model_name.c_str());
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
        ImGui::SetCursorPosX(100);
        if (ImGui::Button("Generate", ImVec2(200, 50))) {
            show_question_input = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::SeparatorText("Please have:");
            ImGui::Text("Your question ready");
            ImGui::Separator();
            ImGui::EndTooltip();
        }

        ImGui::SetCursorPosX(543);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        ImGui::Separator();


        ImGui::End();
        ImGui::PopStyleColor();
    }
    
    // Renders Question Input Window
    void RenderQuestionInputWindow() {
        if (show_question_input) {
            ImGui::Begin("Question Input Window", nullptr, ImGuiWindowFlags_NoCollapse);

            // Static string to persist input across frames
            static std::string inputText;

            auto InputTextWithResize = [](const char* label, std::string& str) -> bool {
                // Resize callback
                auto callback = [](ImGuiInputTextCallbackData* data) -> int {
                    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                        auto* str = static_cast<std::string*>(data->UserData);
                        str->resize(data->BufTextLen);
                        data->Buf = &(*str)[0]; // FIXED: Writable buffer
                    }
                    return 0;
                    };

                // Ensure buffer has room
                if (str.capacity() == 0) str.reserve(256);
                return ImGui::InputText(label, &str[0], str.capacity() + 1,
                    ImGuiInputTextFlags_CallbackResize,
                    callback, &str);
                };


            // Render input field
            InputTextWithResize("##YourQuestion", inputText);

            // Submit button
            if (ImGui::Button("Submit")) {
                output = client.sendPrompt(inputText);
                show_question_output = true;
                show_question_input = false;
                inputText.clear(); // clear input after sending
            }

            ImGui::End();
        }
    }

    // Renders Question Output Window
    void RenderQuestionOutputWindow() {
        if (show_question_output) {
            ImGui::Begin("Question Out Window", nullptr, ImGuiWindowFlags_NoCollapse);
            ImGui::Text("response:");
            std::string cleanOutput;
            for (char c : output) {
                if (isprint(c) || c == '\n') {
                    cleanOutput += c;
                }
            }
            ImGui::Text("%s", cleanOutput.c_str());

            if (ImGui::Button("Finish")) {
                show_question_output = false;
            }

            ImGui::End();
        }
    }


    // Main Render Function for UI
    void RenderUI() {
        RenderApplicationWindow();

        RenderQuestionInputWindow();
        RenderQuestionOutputWindow();
    }

}