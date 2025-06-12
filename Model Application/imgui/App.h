#pragma once
#include <vector>
#include <fstream>
#include <algorithm>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace App {

    // Renders Main Header - called by RenderApplicationWindow()
    void RenderApplicationHeader();

    // Renders Main Window
    void RenderApplicationWindow();

    // Renders Question Input Window
    void RenderQuestionInputWindow();

    // Renders Question Output Window
    void RenderQuestionOutputWindow();

    // Main Render Function for UI
    void RenderUI();

}