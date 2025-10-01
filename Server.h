#pragma once
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <vector>
#include <filesystem>
#include <string>
#include <fstream>

using namespace std;
using namespace sf;
namespace fs = filesystem;

class Server {
private:
    vector<string> files;
    string target_folder_path;

public:
    Server();
    int server_loop(unsigned int& port, string version);
    bool iterate_directory(string &target_folder_path, vector<string> &files);
    vector<char> copy_file(string &file_path);
};