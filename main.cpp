#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <filesystem>
#include <fstream>

using namespace std;
using namespace sf;
namespace fs = std::filesystem;

bool iterate_directory(string& target_folder_path, vector<string>& files) {
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

vector<char> copy_file(string& file_path) {
    ifstream file(file_path, ios::binary);
    file.seekg(0, ios::end);
    size_t size = file.tellg();
    file.seekg(0, ios::beg);

    vector<char>buffer(size);
    file.read(buffer.data(), size);

    return buffer;
}

int server(unsigned int& port, vector<string>& files, string& target_folder_path) {
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
                    cout << "New client connected: " << client->getRemoteAddress() << "\n";
                    selector.add(*client);
                    clients.push_back(move(client));
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

                            if (msg.find("scan") != string::npos and msg.size() > 5) {
                                string new_folder_path = msg.substr(5);
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
                                ++it;
                            }
                            else if (msg.find("scan") != string::npos and msg.size() <= 4) {
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
                        }
                    }
                    else
                        ++it;
                }
            }
        }
    }
}

int client(unsigned int& port, string& target_folder_path) {
    TcpSocket socket;
    socket.connect("127.0.0.1", port);
    socket.setBlocking(false);

    RenderWindow window(VideoMode(900, 600), "Test");
    Event event;
    string type;
    
    Font font;
    font.loadFromFile("src/Fonts/font.ttf");
    Text input_text;
    input_text.setFont(font);
    input_text.setCharacterSize(20);
    input_text.setPosition(10, 10);
    input_text.setString(">> ");
    Text status_text;
    status_text.setFont(font);
    status_text.setCharacterSize(20);
    status_text.setPosition(10, 40);
    Text connection_status_text;
    connection_status_text.setFont(font);
    connection_status_text.setCharacterSize(20);
    connection_status_text.setPosition(10, 570);
    vector<Text> files_text;

    Texture folder_texture;
    folder_texture.loadFromFile("src/Textures/folder.png");
    folder_texture.setSmooth(true);
    Texture file_texture;
    file_texture.loadFromFile("src/Textures/file.png");
    file_texture.setSmooth(true);
    vector<Sprite> file_sprites;

    RectangleShape cursor;
    cursor.setSize(Vector2f(15, 15));
    cursor.setFillColor(Color::White);
    Clock click_delay_clock;

    bool send_message = false;
    string input;
    vector<string> full_file_paths;

    while (window.isOpen()) {
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();
            if (event.type == Event::TextEntered) {
                if (!Keyboard::isKeyPressed(Keyboard::Enter))
                    input += event.text.unicode;
                input_text.setString(">> " + input);
                if (event.text.unicode == '\r' || event.text.unicode == '\n')
                    send_message = true;
            }
        }

        window.clear(Color(60, 60, 60));

        cursor.setPosition(window.mapPixelToCoords(Vector2i(Mouse::getPosition(window).x, Mouse::getPosition(window).y)));

        // Send message
        if (send_message) {
            Packet packet;
            packet << input;
            socket.send(packet);
            input_text.setString(">> "); 
            if (input == "scan" || input.find("scan") == string::npos)
                status_text.setString("Scanning...");
            input.clear();
            send_message = false;
        }

        // Receive reply
        Packet header;
        Socket::Status status = socket.receive(header);
        if (status == Socket::Done) {
            header >> type;

            if (type == "scan") {
                files_text.clear();
                file_sprites.clear();
                full_file_paths.clear();

                int file_count = 0;
                header >> file_count;

                for (int i = 0; i < file_count; i++) {
                    Packet reply;
                    Socket::Status recv_status = socket.receive(reply);
                    if (recv_status != Socket::Done) {
                        cout << "Failed to receive file " << i << ", status: " << recv_status << endl;
                        break; // Stop processing incomplete data
                    }
                    string filepath;
                    string file_type;
                    reply >> filepath >> file_type;
                    full_file_paths.push_back(filepath);
                    cout << "File [" << (i + 1) << "]: " << filepath << "\n";
                    fs::path file_name = filepath;
                    Text file_text;
                    file_text.setFont(font);
                    file_text.setCharacterSize(20);
                    file_text.setPosition(10, 40);
                    file_text.setString(file_name.filename().string());
                    files_text.push_back(file_text);
                    Sprite file_sprite;
                    if (file_type == "folder")
                        file_sprite.setTexture(folder_texture);
                    else if (file_type == "file")
                        file_sprite.setTexture(file_texture);
                    file_sprite.setScale(0.08, 0.08);
                    file_sprites.push_back(file_sprite);
                }
            }
            else if (type == "copy") {
                int total_size = 0;
                header >> total_size;

                cout << "Receiving file of size: " << total_size << " bytes" << endl;

                vector<char> received_data;
                received_data.reserve(total_size);

                // Receive data in chunks
                size_t bytes_received = 0;
                while (bytes_received < total_size) {
                    Packet data_packet;
                    if (socket.receive(data_packet) != Socket::Done) {
                        cerr << "Error: Failed to receive file data" << endl;
                        break;
                    }

                    int chunk_size;
                    data_packet >> chunk_size;

                    for (int i = 0; i < chunk_size; i++) {
                        int byte_value;
                        data_packet >> byte_value;
                        received_data.push_back(static_cast<char>(byte_value));
                        bytes_received++;
                    }
                }

                if (bytes_received == total_size) {
                    cout << "File transfer completed successfully!" << endl;
                    cout << "Save file as: ";
                    string filename;
                    getline(cin, filename);
                    if (!filename.empty()) {
                        ofstream outfile(filename, ios::binary);
                        outfile.write(received_data.data(), received_data.size());
                        cout << "File saved as: " << filename << endl;
                    }
                }
                else {
                    cout << "File transfer incomplete. Received " << bytes_received << " of " << total_size << " bytes" << endl;
                }
            }
            else if (type == "msg") {
                string response;
                header >> response;
                cout << "Server replied: " << response << "\n";
            }
            status_text.setString("");
        }
        else if (status == Socket::Disconnected) {
            connection_status_text.setString("Disconnected");
            connection_status_text.setFillColor(Color(255, 100, 100));
        }
        else {
            connection_status_text.setString("Connected");
            connection_status_text.setFillColor(Color(150, 255, 150));
        }

        window.draw(input_text);
        window.draw(connection_status_text);
        window.draw(status_text);
        window.draw(cursor);
        for (int i = 0; i < files_text.size(); i++) {
            files_text[i].setPosition(50, 40 + (33 * i));
            file_sprites[i].setPosition(10, 40 + (33 * i));
            window.draw(files_text[i]);
            window.draw(file_sprites[i]);

            if (Mouse::isButtonPressed(Mouse::Left) && click_delay_clock.getElapsedTime().asSeconds() > 0.3 && cursor.getGlobalBounds().intersects(files_text[i].getGlobalBounds())) {
                string current_file = files_text[i].getString();
                input = "scan " + full_file_paths[i];
                send_message = true;
                click_delay_clock.restart();
            }
        }
        window.display();
    }
}

int main() {
    char choice;
    unsigned int port = 100;
    string target_folder_path = "C:\\GA(Y)MES\\";
    vector<string> files;

    cout << "Enter 's' for server or 'c' for client: ";
    cin >> choice;

    if (choice == 's') {
        server(port, files, target_folder_path);
    }
    else if (choice == 'c') {
        client(port, target_folder_path);
    }
    else {
        cout << "Invalid choice. Please enter 's' or 'c'.\n";
    }

    return 0;
}