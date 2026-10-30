#include "utilities.hpp"
#include <SDL3/SDL.h>
#include <fstream>
#include <sstream>

std::string readTextFile(const std::string &file_path)//reads the file of the provided path
{
    std::ifstream infile(file_path);
    if (infile.is_open())
    {
        std::stringstream buffer;
        buffer << infile.rdbuf();
        return buffer.str();
    }
    return std::string();
}