#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>

class FileManager {
public:
    FileManager();
    ~FileManager();
    bool writeData(const std::string& app, const std::string& filename, const std::string& data);
    std::string readData(const std::string& app, const std::string& filename);
};

#endif // FILEMANAGER_H
