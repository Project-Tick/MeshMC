// Public domain. It is too generic to meet the threshold of originality defined in the
// 5846 Sayılı Fikir ve Sanat Eserleri Kanunu Madde 1/B (a) bendi.

#include <filesystem>
#include <fstream>
#include <regex>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: shimgen <src_root> <include_root>\n";
        return 1;
    }
    std::filesystem::path srcRoot = argv[1];
    std::filesystem::path includeRoot = argv[2];

    std::regex marker(R"(@meshmc-public:\s*([\w./-]+))");

    for (auto& entry : std::filesystem::recursive_directory_iterator(srcRoot)) {
        if (entry.path().extension() != ".h" && entry.path().extension() != ".hpp") continue;

        std::ifstream in(entry.path());
        std::string line;
        std::smatch m;
        while (std::getline(in, line)) {
            if (std::regex_search(line, m, marker)) {
                std::filesystem::path publicPath = includeRoot / m[1].str();
                std::filesystem::create_directories(publicPath.parent_path());

                std::filesystem::path rel = std::filesystem::relative(entry.path(), publicPath.parent_path());

                std::ofstream out(publicPath);
                out << "#pragma once\n#include \"" << rel.generic_string() << "\"\n";
                break;
            }
        }
    }
}
