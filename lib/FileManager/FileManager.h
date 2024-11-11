#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>

class FileManager {
public:
    FileManager();
    ~FileManager();
    static bool writeData(const std::string& app, const std::string& filename, const std::string& data);
    static bool appendData(const std::string& app, const std::string& filename, const std::string& data);
    static std::string readData(const std::string& app, const std::string& filename);
};

#endif // FILEMANAGER_H
