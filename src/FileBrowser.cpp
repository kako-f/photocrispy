#include "FileBrowser.h"
#include "imgui.h"
#include <algorithm>
#include <unordered_set>
#include <utility>


bool FileBrowser::IsSupportedImage(const fs::path &path) const {
  static const std::unordered_set<std::string> extensions = {
      ".arw", ".dng", ".cr2", ".cr3", ".nef", ".raf", ".rw2", ".orf"};

  std::string ext = path.extension().string();

  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return extensions.find(ext) != extensions.end();
}
bool FileBrowser::Refresh(const fs::path &folder) {
  std::error_code error;
  std::vector<fs::path> refreshedFiles;

  for (fs::directory_iterator it(folder, error), end;
       !error && it != end; it.increment(error)) {
    if (it->is_regular_file(error) && IsSupportedImage(it->path()))
      refreshedFiles.push_back(it->path());
  }

  if (error)
    return false;

  files = std::move(refreshedFiles);
  return true;
}
const fs::path &FileBrowser::GetSelectedFile() const { return selectedFile; }

bool FileBrowser::Draw() {
  bool selectionChanged = false;

  for (const auto &file : files) {
    const bool isSelected = (file == selectedFile);
    const std::string filename = file.filename().string();
    if (ImGui::Selectable(filename.c_str(), isSelected)) {
      selectedFile = file;
      selectionChanged = true;
    }
    // ImGui::TextUnformatted(file.filename().string().c_str());
  }

  return selectionChanged;
}
