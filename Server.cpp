#include "Server.h"

Server::Server() {
#ifdef _WIN32
    target_folder_path = "C:\\";
#elif __linux__
    target_folder_path = "/";
#endif
}

int Server::server_loop(unsigned int& port, string version) {
    #ifdef _WIN32
        system("cls");
    #elif __linux__
        system("clear");
    #endif

    cout << version + " [Server]" << "\n\n";
    cout << "Server ip address: " << IpAddress::getLocalAddress() << "\n";

    TcpListener listener;
    if (listener.listen(port) != Socket::Done) {
        cerr << "Error: Could not listen on port " << port << "\n";
        return 1;
    }
    cout << "Server is listening on port " << port << "...\n";

    vector<unique_ptr<TcpSocket>> clients;
    SocketSelector selector;

    selector.add(listener);

    while (true) {
        if (selector.wait()) {
            if (selector.isReady(listener)) {
                auto client = make_unique<TcpSocket>();
                if (listener.accept(*client) == Socket::Done) {
                    cout << "New client connected: {" << client->getRemoteAddress().getLocalAddress() << "}\n";
                    selector.add(*client);
                    clients.push_back(move(client));
                    cout << "Active clients: [" << clients.size() << "]\n";
                }
            }
            else {
                for (auto it = clients.begin(); it != clients.end();) {
                    TcpSocket& client = **it;
                    if (selector.isReady(client)) {
                        Packet packet;
                        if (client.receive(packet) == Socket::Done) {
                            string msg;
                            packet >> msg;
                            cout << "Client says: " << msg << "\n";

                            if (msg.find("scan server") != string::npos and msg.size() > 12) {
                                string new_folder_path = msg.substr(12);
                                if (iterate_directory(new_folder_path, files)) {
                                    Packet header;
                                    header << "scan" << static_cast<int>(files.size());
                                    client.send(header);

                                    for (size_t i = 0; i < files.size(); i++) {
                                        Packet reply;
                                        string file_type = fs::is_directory(files[i]) ? "folder" : "file";
                                        reply << files[i] << file_type;
                                        client.send(reply);
                                    }
                                }
                                else {
                                    Packet header;
                                    header << "scan" << 1;
                                    client.send(header);

                                    Packet reply;
                                    string file_type = "error";
                                    reply << "Cannot open directory: Persmission denied" << file_type;
                                    client.send(reply);                             
                                }
                                ++it;
                            }
                            else if (msg.find("scan server") != string::npos and msg.size() <= 11) {
                                if (iterate_directory(target_folder_path, files)) {
                                    Packet header;
                                    header << "scan" << static_cast<int>(files.size());
                                    client.send(header);

                                    for (size_t i = 0; i < files.size(); i++) {
                                        Packet reply; 
                                        string file_type = fs::is_directory(files[i]) ? "folder" : "file";
                                        reply << files[i] << file_type;
                                        client.send(reply);
                                    }
                                }
                                ++it;
                            }
                            else if (msg.find("copy") != string::npos && msg.size() > 5) {
                                string file = msg.substr(5);
                                vector<char> buffer = copy_file(file);
                                Packet header;
                                header << "copy" << static_cast<int>(buffer.size());
                                client.send(header);

                                const size_t chunk_size = 8192;
                                for (size_t i = 0; i < buffer.size(); i += chunk_size) {
                                    Packet reply;
                                    size_t chunk_end = min(i + chunk_size, buffer.size());
                                    reply << static_cast<int>(chunk_end - i);
                                    for (size_t j = i; j < chunk_end; j++) {
                                        reply << static_cast<int>(static_cast<unsigned char>(buffer[j]));
                                    }
                                    client.send(reply);
                                }
                                ++it;
                            }
                            else {
                                if (msg.size() != 0) {
                                    Packet reply;
                                    reply << "msg" << ("Message received from server!");
                                    client.send(reply);
                                    ++it;
                                }
                                else {
                                    Packet reply;
                                    reply << "msg" << ("Connected to server!");
                                    client.send(reply);
                                    ++it;
                                }
                            }
                        }
                        else {
                            cout << "Client disconnected.\n";
                            selector.remove(client);
                            it = clients.erase(it);
                            cout << "Active clients: [" << clients.size() << "]\n";
                        }
                    }
                    else
                        ++it;
                }
            }
        }
    }
}

bool Server::iterate_directory(string &target_folder_path, vector<string> &files) {
    // This function scans the server directory and stores all file paths in the 'files' vector
    // so that they can be sent over the network
    
    int line = 0;
    files.clear();
    try {
        for (const auto& entry : fs::directory_iterator(target_folder_path)) {
            if (entry.exists()) {
                line++;
                files.push_back(entry.path().string());
            }
        }
        return true;
    }
    catch (const fs::filesystem_error& e) {
        cout << "[Filesystem error]: " << e.what() << endl; // Added proper error message
        return false;
    }
    catch (const std::exception& e) {
        cout << "[Exception error]: " << e.what() << endl;
        return true; // Return true - normal iterator completion can throw exceptions
    }
}

vector<char> Server::copy_file(string &file_path) {
    // This funciton reads a file iin binary form and returns its contents
    // so that it can be sent over the network

    ifstream file(file_path, ios::binary);
    if (file.is_open() && file.good()) {
        file.seekg(0, ios::end);
        size_t size = file.tellg();
        file.seekg(0, ios::beg);

        vector<char>buffer(size);
        file.read(buffer.data(), size);

        return buffer;
    }
    else {
        vector<char>buffer(0);
        return buffer;
    }
}   