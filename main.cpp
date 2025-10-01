#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <filesystem>
#include <fstream>
#include "Server.h"
#include "Client.h"

using namespace std;
using namespace sf;
namespace fs = std::filesystem;

int main() {
    string version = "Syncy v1.1";
    char choice;
    cout << "Enter 's' for server or 'c' for client: ";
    cin >> choice;

    if (choice == 's') {
        unsigned int port = 25565;
        Server server;
        server.server_loop(port, version);
    }
    else if (choice == 'c') {
       Client client;
       client.client_loop(version);
    }
    else
        cout << "Invalid choice. Please enter 's' or 'c'.\n";

}
