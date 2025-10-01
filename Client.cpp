#include "Client.h"

Client::Client() {
    font.loadFromFile("src/Fonts/font.ttf");
    input_text.setFont(font);
    input_text.setCharacterSize(20);
    input_text.setPosition(10, 10);
    input_text.setString(">> ");
    status_text.setFont(font);
    status_text.setCharacterSize(20);
    status_text.setPosition(25, 50);
    connection_status_text.setFont(font);
    connection_status_text.setCharacterSize(20);
    connection_status_text.setPosition(10, 570);
    file_path_status.setFont(font);
    file_path_status.setCharacterSize(20);
    file_path_status.setPosition(20, 50);
    file_path_status.setFillColor(Color(100, 100, 100));
    file_count_status.setFont(font);
    file_count_status.setCharacterSize(20);
    file_count_status.setPosition(20, 525);
    file_count_status.setFillColor(Color(100, 100, 100));

    folder_texture.loadFromFile("src/Textures/folder.png");
    folder_texture.setSmooth(true);
    file_texture.loadFromFile("src/Textures/file.png");
    file_texture.setSmooth(true);
    error_texture.loadFromFile("src/Textures/error.png");
    error_texture.setSmooth(true);
    file_bounds_texture.loadFromFile("src/Textures/file_bounds.png");
    file_bounds_texture.setSmooth(true);
    select_bar_texture.loadFromFile("src/Textures/select_bar.png");
    select_bar_texture.setSmooth(true);
    arrow_texture.loadFromFile("src/Textures/arrow.png");
    arrow_texture.setSmooth(true);

    cursor.setSize(Vector2f(1, 1));
    cursor.setFillColor(Color::White);

    file_bounds_sprite.setTexture(file_bounds_texture);
    file_bounds_sprite.setPosition(10, 40);

    bounds[0].setPosition(10, 85);
    bounds[0].setSize(Vector2f(300, 433));

    port_input_box.setPosition(350, 250);
    port_input_box.setSize(Vector2f(200, 50));
    port_input_box.setFillColor(Color(50, 50, 50));
    port_input_box.setOrigin(port_input_box.getSize().x / 2, port_input_box.getSize().y / 2);
    port_input_corners[0].setRadius(25);
    port_input_corners[0].setPointCount(30);
    port_input_corners[0].setFillColor(Color(50, 50, 50));
    port_input_corners[1].setRadius(25);
    port_input_corners[1].setPointCount(30);
    port_input_corners[1].setFillColor(Color(50, 50, 50));

    ip_input_box.setPosition(350, 250);
    ip_input_box.setSize(Vector2f(200, 50));
    ip_input_box.setFillColor(Color(50, 50, 50));
    ip_input_box.setOrigin(port_input_box.getSize().x / 2, port_input_box.getSize().y / 2);
    ip_input_corners[0].setRadius(25);
    ip_input_corners[0].setPointCount(30);
    ip_input_corners[0].setFillColor(Color(50, 50, 50));
    ip_input_corners[1].setRadius(25);
    ip_input_corners[1].setPointCount(30);
    ip_input_corners[1].setFillColor(Color(50, 50, 50));

    connect_menu_info_text.setFont(font);
    connect_menu_info_text.setCharacterSize(30);
    connect_menu_info_text.setString("Enter the port number of the\n server you want to connect!");
    connect_menu_info_text.setFillColor(Color(200, 200, 200));
    port_text.setFont(font);
    port_text.setCharacterSize(30);
    port_text.setString(to_string(port));
    port_text.setFillColor(Color(150, 255, 150));
    ip_text.setFont(font);
    ip_text.setCharacterSize(30);
    ip_text.setString(ip_str);
    ip_text.setFillColor(Color(150, 255, 150));

    arrow_sprite.setTexture(arrow_texture);
    arrow_sprite.setScale(0.2, 0.2);
}

void Client::events() {
    while(window.pollEvent(event)) {
        if (event.type == Event::Closed)
            window.close();
        if (current_state == ClientState::handleCommunication) {
            if (event.type == Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.wheel == Mouse::VerticalWheel) {
                    if (event.mouseWheelScroll.delta > 0 && scroll_value < 0)
                        scroll_value++;
                    if (event.mouseWheelScroll.delta < 0)
                        scroll_value--;
                }
            }
            if (event.type == Event::TextEntered && !Keyboard::isKeyPressed(Keyboard::Enter))
                input += event.text.unicode;
            if (Keyboard::isKeyPressed(Keyboard::Backspace))
                input = input.substr(0, input.size() - 1);
            if (Keyboard::isKeyPressed(Keyboard::Backspace) && Keyboard::isKeyPressed(Keyboard::LShift))
                input = "";
            if (Keyboard::isKeyPressed(Keyboard::Enter)) {
                send_message = true;
                click_delay_clock.restart();
            }
        }
        if (current_state == ClientState::establishConnection) {
            if (is_ip_input_active) {
                if (event.type == Event::TextEntered && !Keyboard::isKeyPressed(Keyboard::Enter))
                    ip_str += event.text.unicode;
                if (Keyboard::isKeyPressed(Keyboard::Backspace))
                    ip_str = ip_str.substr(0, ip_str.size() - 1);
                if (Keyboard::isKeyPressed(Keyboard::Backspace) && Keyboard::isKeyPressed(Keyboard::LShift))
                    ip_str = "";
                ip_text.setString(ip_str);
            }
            if (is_port_input_active) {
                if (event.type == Event::TextEntered && !Keyboard::isKeyPressed(Keyboard::Enter) && port_text.getString().getSize() < 5)
                    port_str += event.text.unicode;
                if (Keyboard::isKeyPressed(Keyboard::Backspace))
                    port_str = port_str.substr(0, port_str.size() - 1);
                if (Keyboard::isKeyPressed(Keyboard::Backspace) && Keyboard::isKeyPressed(Keyboard::LShift))
                    port_str = "";
                port_text.setString(port_str);
            }
        }
    }
}

void Client::client_loop(string version) {
    window.create(VideoMode(900, 600), version + " [Client]");

    while(window.isOpen()) {
        events();

        window.clear(Color(60, 60, 60));

        cursor.setPosition(window.mapPixelToCoords(Vector2i(Mouse::getPosition(window).x, Mouse::getPosition(window).y)));
        establishConnection();
        handleCommunication();

        window.display();
    }
}

void Client::establishConnection() {
    if (current_state != ClientState::establishConnection)
        return;

    ip_input_box.setPosition((window.getSize().x / 2), 290);
    ip_input_corners[0].setPosition(ip_input_box.getPosition().x - 125, ip_input_box.getPosition().y - 25);
    ip_input_corners[1].setPosition(ip_input_box.getPosition().x + 75, ip_input_box.getPosition().y - 25);
    port_input_box.setPosition((window.getSize().x / 2), 350);
    port_input_corners[0].setPosition(port_input_box.getPosition().x - 125, port_input_box.getPosition().y - 25);
    port_input_corners[1].setPosition(port_input_box.getPosition().x + 75, port_input_box.getPosition().y - 25);
    connect_menu_info_text.setPosition((window.getSize().x / 2) - (connect_menu_info_text.getLocalBounds().width / 2), 160);
    ip_text.setPosition(ip_input_box.getPosition().x - (ip_text.getLocalBounds().width / 2), ip_input_box.getPosition().y - (ip_text.getLocalBounds().height / 2) - 10);
    port_text.setPosition(port_input_box.getPosition().x - (port_text.getLocalBounds().width / 2), port_input_box.getPosition().y - (port_text.getLocalBounds().height / 2) - 10);

    // Arrow animation
    static float scale = 0;
    if (cursor.getGlobalBounds().intersects(arrow_sprite.getGlobalBounds()) &&
        animation_clock.getElapsedTime().asSeconds() > 0.01 && scale < 0.1) {
        scale += 0.01;
        animation_clock.restart();
    }
    else if (!cursor.getGlobalBounds().intersects(arrow_sprite.getGlobalBounds()) &&
        animation_clock.getElapsedTime().asSeconds() > 0.01 && scale >= 0.01) {
        scale -= 0.01;
        animation_clock.restart();
    }
    arrow_sprite.setPosition(window.getSize().x / 2 - 20 - (scale * 100), port_input_box.getPosition().y / 2 + 220);
    arrow_sprite.setScale(0.2 + scale, 0.2 + scale);

    // Input box selection
    if (cursor.getGlobalBounds().intersects(ip_input_box.getGlobalBounds()) && Mouse::isButtonPressed(Mouse::Left)) {
        is_port_input_active = false;
        is_ip_input_active = true;
    }
    if (cursor.getGlobalBounds().intersects(port_input_box.getGlobalBounds()) && Mouse::isButtonPressed(Mouse::Left)) {
        is_port_input_active = true;
        is_ip_input_active = false;
    }

    for (char c : port_str) {
        if (isalpha(c)) {
            valid_port = false;
            break;
        }
        valid_port = true;
        port = stoi(port_str);
    }
    if ((Keyboard::isKeyPressed(Keyboard::Enter) && port_str.size() > 0 && valid_port) ||
        Mouse::isButtonPressed(Mouse::Left) && cursor.getGlobalBounds().intersects(arrow_sprite.getGlobalBounds())) {
        socket.setBlocking(false);
        if (socket.connect(ip_str, port)) {
            current_state = ClientState::handleCommunication;
            cout << "Connected to server on port " << port << "!\n";
        }
    }

    // Port text color
    if (valid_port)
        port_text.setFillColor(Color(150, 255, 150));
    else
        port_text.setFillColor(Color(255, 100, 100));

    window.draw(ip_input_box);
    window.draw(ip_input_corners[0]);
    window.draw(ip_input_corners[1]);
    window.draw(port_input_box);
    window.draw(port_input_corners[0]);
    window.draw(port_input_corners[1]);
    window.draw(connect_menu_info_text);
    window.draw(ip_text);
    window.draw(port_text);
    if(valid_port)
        window.draw(arrow_sprite);
}

void Client::handleCommunication() {
    if (current_state != ClientState::handleCommunication)
        return;

    // Caret animation
    if (caret_clock.getElapsedTime().asSeconds() < 0.5)
        input_text.setString(">> " + input + "_");
    if (caret_clock.getElapsedTime().asSeconds() >= 0.5)
        input_text.setString(">> " + input);
    if(caret_clock.getElapsedTime().asSeconds() > 1)
        caret_clock.restart();

    // Check connection every 5 seconds
    if (connection_status_clock.getElapsedTime().asSeconds() > 5) {
        send_message = true;
        check_connection = true;
        connection_status_clock.restart();
    }

    // Send message
    if (send_message) {
        Packet packet;
        if (check_connection) {
            packet << "Checking connection...";
        }
        else {
            packet << input;
            input_text.setString(">> " + input);
            if (input == "scan" || input.find("scan") != string::npos) {
                status_text.setString("Scanning...");
                input.clear();
                files_text.clear();
                file_sprites.clear();
                select_bars.clear();
                file_types.clear();
                full_file_paths.clear();
                file_path_status.setString("");
                file_count_status.setString("");
            }
            if (input == "copy" || input.find("copy") != string::npos) {
                status_text.setString("Copying...");
                int pos = input.find_last_of("/\\");
                if (pos != string::npos)
                    copied_file_name = input.substr(pos + 1);
            }
        }
        socket.send(packet);
        receive_message = true;
        send_message = false;
        check_connection = false;
    }

    // Receive reply
    Packet header;
    Socket::Status status;
    if (receive_message) {
        socket.setBlocking(true);
        status = socket.receive(header);
        receive_message = false;
    }
    if (status == Socket::Done) {
        header >> type;
        // What type of message is it?
        if (type == "scan") {
            int file_count = 0;
            header >> file_count;

            for (int i = 0; i < file_count; i++) {
                Packet reply;
                Socket::Status recv_status = socket.receive(reply);
                if (recv_status != Socket::Done) {
                    cout << "Failed to receive file " << i << ", status: " << recv_status << endl;
                    break;
                }
                string filepath;
                string file_type;
                reply >> filepath >> file_type;
                file_types.push_back(file_type);
                full_file_paths.push_back(filepath);
                fs::path file_name = filepath;
                files_text.push_back(Text());
                files_text.back().setFont(font);
                files_text.back().setCharacterSize(20);
                files_text.back().setPosition(10, 40);
                files_text.back().setString(file_name.filename().string());
                Sprite file_sprite;
                if (file_type == "folder")
                    file_sprite.setTexture(folder_texture);
                else if (file_type == "file")
                    file_sprite.setTexture(file_texture);
                else if (file_type == "error")
                    file_sprite.setTexture(error_texture);
                file_sprite.setScale(0.07, 0.07);
                file_sprites.push_back(file_sprite);
                select_bars.push_back(Sprite());
                select_bars.back().setTexture(select_bar_texture);
                file_path_status.setString(" Current path: " + file_name.parent_path().string());
                file_count_status.setString("Files: " + to_string(file_count));
            }
            scroll_value = 0;
        }
        else if (type == "copy") {
            int total_size = 0;
            header >> total_size;
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
                ofstream outfile(copied_file_name, ios::binary);
                if (!outfile.is_open())
                    cout << "Error: Could not create file '" << copied_file_name << "'" << endl;
                outfile.write(received_data.data(), received_data.size());
                if (outfile.bad())
                    cout << "Error: Failed to write file data" << endl;
                outfile.close();
                cout << "File saved as: " << copied_file_name << endl;
            }
            else
                cout << "File transfer incomplete. Received " << bytes_received << " of " << total_size << " bytes" << endl;
        }
        else if (type == "msg") {
            string response;
            header >> response;
            cout << "Server replied: " << response << "\n";
            connection_status_text.setString("Connected");
            connection_status_text.setFillColor(Color(150, 255, 150));
        }
        status_text.setString("");
        socket.setBlocking(false); 
    }
    else if (status == Socket::Disconnected) {
        connection_status_text.setString("Disconnected");
        connection_status_text.setFillColor(Color(255, 100, 100));
    }

    // Scan server directory
    for (int i = 0; i < files_text.size(); i++) {
        if (click_delay_clock.getElapsedTime().asSeconds() > 0.3 && 
            cursor.getGlobalBounds().intersects(select_bars[i].getGlobalBounds()) && 
            files_text[i].getGlobalBounds().intersects(bounds[0].getGlobalBounds())) {
            if (Mouse::isButtonPressed(Mouse::Left) && file_types[i] != "file") {
                string current_file = files_text[i].getString();
                input = "scan server " + full_file_paths[i];
                send_message = true;
                click_delay_clock.restart();
            }
        }
    }

    window.draw(file_bounds_sprite);
    window.draw(input_text);
    window.draw(connection_status_text);
    window.draw(status_text);
    window.draw(file_path_status);
    window.draw(file_count_status);
    for (int i = 0; i < files_text.size(); i++) {
        files_text[i].setPosition(63, 90 + (36 * (i + scroll_value)));
        file_sprites[i].setPosition(23, 90 + (36 * (i + scroll_value)));
        select_bars[i].setPosition(17, 85 + (36 * (i + scroll_value)));

        if (click_delay_clock.getElapsedTime().asSeconds() > 0.3 && cursor.getGlobalBounds().intersects(select_bars[i].getGlobalBounds()) && files_text[i].getGlobalBounds().intersects(bounds[0].getGlobalBounds())) {
            window.draw(select_bars[i]);
            files_text[i].setPosition(73, 90 + (36 * (i + scroll_value)));
            file_sprites[i].setPosition(33, 90 + (36 * (i + scroll_value)));
        }
        if (file_sprites[i].getGlobalBounds().intersects(bounds[0].getGlobalBounds())) {
            window.draw(files_text[i]);
            window.draw(file_sprites[i]);
        }
    }
    //window.draw(bounds[0]);
}