#include <iostream>
#include <list>
#include "ChatServer.h"

ChatServer::ChatServer(SOCKET s, sockaddr_in addr) {
    m_Socket = s;
    m_Sockaddr = addr;
}

void ChatServer::setCurrentPeer(int id) {
    m_CurrentPeer = id;
}

int ChatServer::getCurrentPeer() const {
    return m_CurrentPeer;
}


string ChatServer::getFullName(int id) const {
    auto it = m_Users.find(id);
    if(it == m_Users.cend()) return "";
    else return (it->second + "#" + to_string(id));
}

int ChatServer::login(string username) {
    // username.size() is guaranteed to be less than 33
    string msg = username + "\n";
    //uint16_t len = (uint16_t) htons((u_short) username.size());
    msg.insert(0, 1, (char) CODE_LOGINREQUEST);
    //msg.insert(1, (char*) &len, 2);

    int r = usendto(m_Socket, m_Sockaddr, msg.c_str(), 0);
    if(r < 0) {
        printf("Failed to login: %s\n", strerror(r));
        return -1;
    }
    if(r != msg.size() + TS_EXTRA_LENGTH) {
        printf("sendMessage: Not all bytes are sent!");
        return -2;
    }
    return 0;
}

int ChatServer::logout() {
    char code = CODE_LOGOUTREQUEST;
    int r = usendto(m_Socket, m_Sockaddr, &code, 1);
    if(r < 0) {
        printf("Failed to logout: %s\n", strerror(r));
        return -1;
    }
    //стоит ли ждать ответ от сервера? а в UDP?
    m_Users.clear();
    return 0;
}

int ChatServer::receiveUsersListPacket() {
    char id_buf [10]; //max 10 digits in a 32-bit id
    char name_buf [MAX_USERNAME_LENGTH];
    int id;
    int bnum;
    int cnt = 0;

    while(1) {
        bnum = readline(m_Socket, id_buf, sizeof(id_buf));
        if (bnum < 0) {
            cerr << "UsersList packet corrupted: no id" << endl;
            return -1;
        } else if (strcmp(id_buf,"\4\n") == 0) {
            break;
        }
        id = atoi(id_buf);
        bnum = readline(m_Socket, name_buf, sizeof(name_buf));
        if (bnum <= 0) {
            cerr << "UsersList packet corrupted: no name for id" << endl;
            return -2;
        }
        name_buf[bnum-1] = '\0';
        m_Users[id] = string(name_buf);
        cnt++;
    }
    return cnt;
}

int ChatServer::receiveUsersList() {
    char code;
    readn(m_Socket, &code, 1);
    if(code == CODE_LOGINANSWER) {
        if(receiveUsersListPacket() < 1) {
            cerr << "Error occurred while receiving users list!" << endl;
        }
    } else {
        cout << "Inappropriate receiveUsersList() call." << endl;
        return -1;
    }
    return 0;
}

void ChatServer::showUsersList() {
    cout << m_Users.size() << " user(s) online:" << endl;
    list<string> vs;
    unordered_map<int, string>::const_iterator it;
    for (it = m_Users.cbegin(); it != m_Users.cend(); it++) {
        if(!it->second.empty()) vs.push_back(it->second + "#" + to_string(it->first));
        else vs.push_back("#" + to_string(it->first) + " (Public room)");
    }
    vs.sort();
    for(string str : vs) {
        cout << str << endl;
    }
}

string ChatServer::startChat(string peer) {
    unordered_map<int, string>::iterator it = m_Users.begin();
    if(peer.find('#') != string::npos) {
        //Search for the id
        string str_id = peer.substr(1);
        int id = atoi(str_id.c_str());
        it = m_Users.find(id);
    } else { //Search for the name
        for ( ; it != m_Users.end(); ++it) {
            if (it->second == peer) break;
        }
    }
    if(it != m_Users.end()) setCurrentPeer(it->first);
    return (it == m_Users.end() ? "" : (it->second + "#" + to_string(it->first)));
}

int ChatServer::sendMessage(string text) {
    //string msg = text;
    //uint16_t len = (uint16_t) htons((u_short) text.size());
    //msg.insert(0, (char*) &len, 2);
    string msg = to_string(getCurrentPeer()) + "\n" + text + "\n";
    msg.insert(0, 1, (char) CODE_INMSG);
    int r = usendto(m_Socket, m_Sockaddr, msg.c_str(), 0);
    if(r < 0) {
        printf("Failed to send message: %s\n", strerror(r));
        return -1;
    }
    if(r != msg.size() + TS_EXTRA_LENGTH) {
        printf("sendMessage: Not all bytes are sent!");
        return -2;
    }
    return 0;
}

bool ChatServer::receiveMessage() {
    char buf [MAX_MSG_LENGTH+1];
    char id_buf [10]; //max 10 digits in a 32-bit id
    int id;

    int r = readline(m_Socket, id_buf, sizeof(id_buf));
    if(r < 0) {
        cerr << "Failed to extract user id from incoming message!" << endl;
    }
    id = atoi(id_buf);
    r = readline(m_Socket, buf, sizeof(buf));
    if(r < 0) {
        cerr << "Failed to read incoming message!" << endl;
    } else if (r == 0) {
        //?
    } else {
        //buf[r] = '\0';
        m_Pending[id].push_back(string(buf));
        return true;
    }
    return false;
}

void ChatServer::showMessage(int id_from) {
    auto it = m_Pending.find(id_from);
    if(it == m_Pending.cend()) return;
    if(it->second.empty()) return;

    string& name = m_Users.at(it->first);
    for(const string& msg : it->second) {
        if(!name.empty()) cout << "[ " << name << " ]: " << msg;
        else cout << msg;
    }
    it->second.clear();
}

string ChatServer::getPendingList() const {
    string list;
    for(auto&& item : m_Pending) {
        if(m_Users.at(item.first).empty()) continue;
        if(!item.second.empty()) list += getFullName(item.first) + ", ";
    }
    if(list.empty()) return "";
    list.pop_back();
    list.pop_back();
    return list;
}

string ChatServer::receiveServerMessage() {
    char buf [MAX_MSG_LENGTH+1];

    int r = readline(m_Socket, buf, sizeof(buf));
    if(r < 0) {
        cerr << "Failed to read server message!" << endl;
    } else if (r == 0) {
        //?
        return "";
    } else {
        //buf[r] = '\0';
        return string(buf);
    }
    return "";
}

bool ChatServer::receiveLoginNotification() {
    auto it1 = m_Users.find(getCurrentPeer());
    if(receiveUsersListPacket() < 1) {
        cerr << "Error occurred while receiving users list from notification!" << endl;
    }
    auto it2 = m_Users.find(getCurrentPeer());
    return (it1 == m_Users.cend() && it2 != m_Users.cend());
}

bool ChatServer::receiveLogoutNotification() {
    auto it1 = m_Users.find(getCurrentPeer());

    char id_buf [10]; //max 10 digits in a 32-bit id
    int id;
    int bnum;

    while(1) {
        bnum = readline(m_Socket, id_buf, sizeof(id_buf));
        if (bnum < 0) {
            cerr << "UsersList packet corrupted: no id" << endl;
            return -1;
        } else if (strcmp(id_buf,"\4\n") == 0) {
            break;
        }
        id = atoi(id_buf);
        m_Users.erase(id);
        m_Pending.erase(id); // Unread messages are deleted (in this version)
    }

    auto it2 = m_Users.find(getCurrentPeer());
    return (it1 != m_Users.cend() && it2 == m_Users.cend());
}

bool ChatServer::sendReqHeartbeat() const {
    char code = CODE_CLIHEARTBEAT;
    int r = usendto(m_Socket, m_Sockaddr, &code, 1);
    if(r < 0) {
        printf("Failed to send heartbeat: %s\n", strerror(r));
        return false;
    }
    return true;
}

bool ChatServer::sendAnsHeartbeat() const {
    char code = CODE_SRVHEARTBEAT;
    int r = usendto(m_Socket, m_Sockaddr, &code, 1);
    if(r < 0) {
        printf("Failed to send heartbeat: %s\n", strerror(r));
        return false;
    }
    return true;
}