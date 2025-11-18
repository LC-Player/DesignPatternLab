#include "TreeDrawer.h"
#include <filesystem>

#include "Outputer.h"

void DrawDirTree(const std::string& dirpath, const std::string& prefix) {
    try {
        std::vector<std::filesystem::directory_entry> entries;
        for (const auto& entry : std::filesystem::directory_iterator(dirpath)) {
            entries.push_back(entry);
        }
        for (int x = 0; x < entries.size(); ++x) {
            auto& entry = entries[x];
            Outputer::Out() << prefix;
            auto dirStr = entry.path().string();
            auto pref = x == entries.size()-1 ? "└── " : "├── " ;
            Outputer::Out() << pref << dirStr.substr(dirStr.find_last_of("\\/") + 1) << std::endl;

            if (std::filesystem::is_directory(entry)) {
                DrawDirTree(dirStr, x == entries.size()-1 ? prefix + "    " : prefix + "│   ");
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
