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

class Client {
private:
    RenderWindow window;
    Event event;

    TcpSocket socket;

    int scroll_value = 0;
    unsigned int port = 25565;

    bool valid_port = false;
    bool send_message = false;
    bool receive_message = false;
    bool check_connection = false;
    bool is_ip_input_active = true;
    bool is_port_input_active = false;

    Font font;

    string port_str = "25565";
    string ip_str = "127.0.0.1";
    string type;
    string copied_file_name;
    string input;
    vector<string> full_file_paths;
    vector<string> file_types;

    Text input_text;
    Text status_text;
    Text connection_status_text;
    Text file_path_status;
    Text file_count_status;
    Text connect_menu_info_text;
    Text port_text;
    Text ip_text;
    vector<Text> files_text;

    Texture folder_texture;
    Texture file_texture;
    Texture error_texture;
    Texture file_bounds_texture;
    Texture select_bar_texture;
    Texture arrow_texture;

    Sprite file_bounds_sprite;
    Sprite arrow_sprite;
    vector<Sprite> file_sprites;
    vector<Sprite> select_bars;

    RectangleShape cursor;
    RectangleShape bounds[1];
    RectangleShape ip_input_box;
    RectangleShape port_input_box;

    CircleShape ip_input_corners[2];
    CircleShape port_input_corners[2];

    Clock click_delay_clock;
    Clock connection_status_clock;
    Clock animation_clock;
    Clock caret_clock;

    enum class ClientState { establishConnection, handleCommunication };
    ClientState current_state = ClientState::establishConnection;

public:
    Client();
    void events();
    void client_loop(string version);
    void establishConnection();
    void handleCommunication();
};