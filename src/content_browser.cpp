#include "imgui.h"
#include "content_browser.h"
#include <filesystem>

namespace ContentBrowser
{
    RawBrowser::RawBrowser()
    {
        baseDirectory = "D:\\Fotos\\2025\\2025-05-09";
        currentDirectory = baseDirectory;
    }
    void RawBrowser::browseImages()
    {

        ImGui::Begin("Photos");
        if (currentDirectory != std::filesystem::path(baseDirectory))
        {
            if (ImGui::Button("<-"))
            {
                currentDirectory = currentDirectory.parent_path();
            }
        }
        static float padding = 16.0f;
        static float thumbnailSize = 128.0f;
        float cellSize = thumbnailSize + padding;

        for (auto &directoryEntry : std::filesystem::directory_iterator(currentDirectory))
        {
            const auto &path = directoryEntry.path();
            std::string filenameString = path.filename().string();
            ImGui::TextWrapped(filenameString.c_str());
        }

        ImGui::End();
    }

}