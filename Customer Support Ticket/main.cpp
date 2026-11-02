
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <winsock2.h>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

struct Ticket {
    int id;
    string name, email, subject;
    string description, priority, status;
};

string decode(string s) {
    string result = "";

    for (int i = 0; i < s.length(); i++) {
        if (s[i] == '+') {
            result += ' ';
        }
        else if (s[i] == '%' && i + 2 < s.length()) {
            int value;
            stringstream ss;
            ss << hex << s.substr(i + 1, 2);
            ss >> value;
            result += char(value);
            i += 2;
        }
        else {
            result += s[i];
        }
    }

    return result;
}

string getValue(string data, string key) {
    stringstream ss(data);
    string part;

    while (getline(ss, part, '&')) {
        int pos = part.find('=');

        if (pos != string::npos) {
            if (part.substr(0, pos) == key) {
                return decode(part.substr(pos + 1));
            }
        }
    }

    return "";
}

string safeText(string s) {
    string result = "";

    for (char c : s) {
        if (c == '&') result += "&amp;";
        else if (c == '<') result += "&lt;";
        else if (c == '>') result += "&gt;";
        else if (c == '"') result += "&quot;";
        else if (c == '\'') result += "&#39;";
        else result += c;
    }

    return result;
}

void saveTicket(Ticket t) {
    ofstream file("tickets.txt", ios::app);

    file << t.id << "|"
         << t.name << "|"
         << t.email << "|"
         << t.subject << "|"
         << t.description << "|"
         << t.priority << "|"
         << t.status << endl;

    file.close();
}

int nextID() {
    ifstream file("tickets.txt");

    Ticket t;
    int id = 100;

    while (file >> t.id) {
        id = t.id;
        file.ignore(10000, '\n');
    }

    file.close();

    return id + 1;
}

string page(string content) {
    return "<!DOCTYPE html><html><head>"
           "<title>Customer Support</title>"
           "<link rel='stylesheet' href='/style.css'>"
           "</head><body><header>"
           "<h1>Customer Support System</h1>"
           "</header><section>" + content +
           "<p><a href='/'>Go Back Home</a></p>"
           "</section></body></html>";
}

string showTickets() {
    ifstream file("tickets.txt");

    Ticket t;
    string result = "<h2>All Customer Tickets</h2>";

    result += "<table><tr><th>ID</th>"
              "<th>Name</th><th>Subject</th>"
              "<th>Priority</th><th>Status</th></tr>";

    int count = 0;

    while (file >> t.id) {
        file.ignore(1);

        getline(file, t.name, '|');
        getline(file, t.email, '|');
        getline(file, t.subject, '|');
        getline(file, t.description, '|');
        getline(file, t.priority, '|');
        getline(file, t.status);

        result += "<tr><td>" + to_string(t.id) +
                  "</td><td>" + safeText(t.name) +
                  "</td><td>" + safeText(t.subject) +
                  "</td><td>" + safeText(t.priority) +
                  "</td><td>" + safeText(t.status) +
                  "</td></tr>";

        count++;
    }

    file.close();

    result += "</table>";

    if (count == 0) {
        result += "<p>No tickets available.</p>";
    }

    return page(result);
}

string searchTicket(int id) {
    ifstream file("tickets.txt");

    Ticket t;
    string result = "<h2>Ticket Search Result</h2>";
    bool found = false;

    while (file >> t.id) {
        file.ignore(1);

        getline(file, t.name, '|');
        getline(file, t.email, '|');
        getline(file, t.subject, '|');
        getline(file, t.description, '|');
        getline(file, t.priority, '|');
        getline(file, t.status);

        if (t.id == id) {
            result += "<p>Ticket ID: " + to_string(t.id) + "</p>";
            result += "<p>Name: " + safeText(t.name) + "</p>";
            result += "<p>Email: " + safeText(t.email) + "</p>";
            result += "<p>Subject: " + safeText(t.subject) + "</p>";
            result += "<p>Problem: " + safeText(t.description) + "</p>";
            result += "<p>Priority: " + safeText(t.priority) + "</p>";
            result += "<p>Status: " + safeText(t.status) + "</p>";

            found = true;
            break;
        }
    }

    file.close();

    if (!found) {
        result += "<p>Ticket Not Found!</p>";
    }

    return page(result);
}

bool updateTicket(int id, string status) {
    ifstream file("tickets.txt");
    ofstream temp("temp.txt");

    Ticket t;
    bool found = false;

    while (file >> t.id) {
        file.ignore(1);

        getline(file, t.name, '|');
        getline(file, t.email, '|');
        getline(file, t.subject, '|');
        getline(file, t.description, '|');
        getline(file, t.priority, '|');
        getline(file, t.status);

        if (t.id == id) {
            t.status = status;
            found = true;
        }

        temp << t.id << "|"
             << t.name << "|"
             << t.email << "|"
             << t.subject << "|"
             << t.description << "|"
             << t.priority << "|"
             << t.status << endl;
    }

    file.close();
    temp.close();

    if (found) {
        remove("tickets.txt");
        rename("temp.txt", "tickets.txt");
    }
    else {
        remove("temp.txt");
    }

    return found;
}

string readFile(string name) {
    ifstream file(name);
    stringstream buffer;

    buffer << file.rdbuf();

    return buffer.str();
}

void sendResponse(SOCKET client, string content,
                  string type = "text/html") {
    string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + type + "; charset=UTF-8\r\n"
        "Content-Length: " + to_string(content.size()) + "\r\n"
        "Connection: close\r\n\r\n" + content;

    send(client, response.c_str(), response.size(), 0);
}

int main() {

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    bind(server, (sockaddr*)&address, sizeof(address));
    listen(server, 5);

    cout << "Customer Support Server Started!\n";
    cout << "Open http://localhost:8080 in your browser\n";

    while (true) {

        SOCKET client = accept(server, NULL, NULL);

        if (client == INVALID_SOCKET) {
            continue;
        }

        string request = "";
        char buffer[4096];

        int headerEnd = -1;
        int contentLength = 0;

        while (true) {
            int bytes = recv(client, buffer, sizeof(buffer), 0);

            if (bytes <= 0) break;

            request.append(buffer, bytes);

            headerEnd = request.find("\r\n\r\n");

            if (headerEnd != -1) {
                string headers = request.substr(0, headerEnd);
                size_t pos = headers.find("Content-Length:");

                if (pos != string::npos) {
                    contentLength = stoi(
                        headers.substr(pos + 15)
                    );
                }

                if (request.size() >=
                    headerEnd + 4 + contentLength) {
                    break;
                }
            }
        }

        string method = "";
        string path = "";

        stringstream firstLine(request.substr(0,
                                  request.find("\r\n")));

        firstLine >> method >> path;

        string body = "";

        if (headerEnd != -1) {
            body = request.substr(headerEnd + 4,
                                  contentLength);
        }

        if (method == "GET" && path == "/") {
            sendResponse(client, readFile("index.html"));
        }

        else if (method == "GET" && path == "/style.css") {
            sendResponse(client, readFile("style.css"),
                         "text/css");
        }

        else if (method == "GET" && path == "/tickets") {
            sendResponse(client, showTickets());
        }

        else if (method == "POST" && path == "/create") {

            Ticket t;

            t.id = nextID();
            t.name = getValue(body, "name");
            t.email = getValue(body, "email");
            t.subject = getValue(body, "subject");
            t.description = getValue(body, "description");
            t.priority = getValue(body, "priority");
            t.status = "Pending";

            saveTicket(t);

            string result =
                "<h2>Ticket Created Successfully!</h2>"
                "<p>Your Ticket ID is: " +
                to_string(t.id) + "</p>";

            sendResponse(client, page(result));
        }

        else if (method == "POST" && path == "/search") {

            int id = 0;

            try {
                id = stoi(getValue(body, "ticket_id"));
            }
            catch (...) {
                id = 0;
            }

            sendResponse(client, searchTicket(id));
        }

        else if (method == "POST" && path == "/update") {

            int id = 0;

            try {
                id = stoi(getValue(body, "ticket_id"));
            }
            catch (...) {
                id = 0;
            }

            string status = getValue(body, "status");

            string result;

            if (status == "Pending" ||
                status == "In Progress" ||
                status == "Resolved") {

                if (updateTicket(id, status)) {
                    result = "<h2>Status Updated Successfully!</h2>";
                }
                else {
                    result = "<h2>Ticket Not Found!</h2>";
                }
            }
            else {
                result = "<h2>Invalid Status!</h2>";
            }

            sendResponse(client, page(result));
        }

        else {
            sendResponse(client, page("<h2>Page Not Found!</h2>"));
        }

        closesocket(client);
    }

    closesocket(server);
    WSACleanup();

    return 0;
}
