#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

class FileBrowser
{
private:
    std::vector<std::filesystem::path> files;
    bool IsSupportedImage(const fs::path &path) const;
    fs::path selectedFile;

public:
    const fs::path& GetSelectedFile() const;
    void Refresh(const std::filesystem::path &folder);
    bool Draw();

};
