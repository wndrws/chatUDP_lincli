#ifdef __cplusplus
extern "C" {
#endif
char* program_name;
#ifdef __cplusplus
}
#endif

#include <iostream>
#include <queue>
#include <pthread.h>
#include <fcntl.h>
#include "etcp.h"
#include "ChatServer.h"
#include "automutex.h"
#include "Data.h"

using namespace std;

ChatServer chatServer;
queue<string> MessagesToSend;
queue<string> MessagesToAck;
static CAutoMutex autoMutex;
volatile bool bgThreadAlive;
volatile bool checkOnline = true;
volatile bool checkPending = false;
bool blockingMode = true;
bool awaitingAck = false;
void showHelp();

Data dataFromServer;

pthread_t bgThread;
void* BackgroundThread(void* data) {
    SOCKET s = *((SOCKET*) data);
    string msg;
    unsigned char code = 255;
    int rcvdb;
    bool ok = true;
    int mode = 0;
    int hbcnt = 0;
    int ackcnt = 0;
    unsigned int timeQuantum = 200; // sleep time between readings from non-blocking socket (ms)
    unsigned int timeHb = 10000; // time to send the first heartbeat (ms)
    unsigned int ackTimeout = 5000;

    while(ok && bgThreadAlive) {
        if (!MessagesToSend.empty()) {
            SCOPE_LOCK_MUTEX(autoMutex.get());
            if (!MessagesToSend.empty()) {
                msg = MessagesToSend.front();
                int r = chatServer.sendMessage(msg);
                if (r < 0) {
                    cout << "Retrying..." << endl;
                } else {
                    MessagesToAck.push(msg);
                    MessagesToSend.pop();
                    awaitingAck = true;
                }
            }
        }
        // Check incoming messages
        if(checkPending && chatServer.getCurrentPeer() == 0
                && !chatServer.getPendingList().empty()) {
            cout << "[ " << "You have incoming message(s) from "
                 << chatServer.getPendingList() << " ]" << endl;
            checkPending = false;
        }
        // Receive packets
        if(mode == 0) {
            mode = 1;
            fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK); // Make socket non-blocking
        }
        blockingMode = false;
        rcvdb = readn(s, (char*) &code, 1);
        if (rcvdb == 0) {
            cout << "[ Server closed the connection ]" << endl;
            break;
        } else if(rcvdb < 0) {
            int error = errno;
            if(error == EWOULDBLOCK || error == EAGAIN || error == EINTR) {
                // It's ok, continue doing job after some time
                usleep(timeQuantum*1000); // sleep for 0.2 seconds
                if(++hbcnt == timeHb/timeQuantum) chatServer.sendReqHeartbeat();
                else if(hbcnt == 2*timeHb/timeQuantum) chatServer.sendReqHeartbeat();
                else if(hbcnt == 3*timeHb/timeQuantum) chatServer.sendReqHeartbeat();
                else if(hbcnt == 4*timeHb/timeQuantum) {
                    cerr << "Error: server is not responding.\n" << endl;
                    break;
                }
                if(awaitingAck && !MessagesToAck.empty()) {
                    if(++ackcnt == ackTimeout/timeQuantum) {
                        cout << "[ Your message probably hasn't reached the server ]" << endl;
                        cout << MessagesToAck.front() << endl;
                        cout << "[ Please try to send it again ]" << endl;
                        MessagesToAck.pop();
                        if(MessagesToAck.empty()) awaitingAck = false;
                        ackcnt = 0;
                    }
                }
            } else {
                cerr << "Error: reading from socket " << s << endl;
                break;
            }
        } else {
            mode = 0;
            fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) & !O_NONBLOCK); // Make socket blocking again
            blockingMode = true;
            switch(code) {
                case CODE_OUTMSG: // Incoming message from some client
                    checkPending = chatServer.receiveMessage();
                    chatServer.showMessage(chatServer.getCurrentPeer());
                    break;
                case CODE_SRVERR:
                    ok = false;
                    // no break here
                case CODE_SRVMSG:
                    cout << "[ Message from server ]" << endl;
                    cout << chatServer.receiveServerMessage();
                    cout << "[ End of message ]" << endl;
                    break;
                case CODE_FORCEDLOGOUT:
                    cout << "[ You were forced to logout by server ]" << endl;
                    ok = false;
                    break;
                case CODE_LOGINNOTIFY:
                    if(chatServer.receiveLoginNotification()) {
                        cout << "[ Your peer has come online ]" << endl;
                    } else if(checkOnline) {
                        if(chatServer.getCurrentPeer() == 0)
                        cout << "[ Some users have come online - "
                             << "type /refresh to see the whole list ]" << endl;
                        checkOnline = false;
                    }
                    break;
                case CODE_LOGOUTNOTIFY:
                    if(chatServer.receiveLogoutNotification()) {
                        cout << "[ Your peer has gone offline ]" << endl;
                    } else if(checkOnline) {
                        if(chatServer.getCurrentPeer() == 0)
                        cout << "[ Some users have gone offline - "
                             << "type /refresh to see the whole list ]" << endl;
                        checkOnline = false;
                    }
                    break;
                case CODE_CLIHEARTBEAT:
                    hbcnt = 0;
                    break;
                case CODE_SRVHEARTBEAT:
                    if(!chatServer.sendAnsHeartbeat()) ok = false;
                    break;
                case CODE_ACK:
                    ackcnt = 0;
                    if(awaitingAck && !MessagesToAck.empty()) MessagesToAck.pop();
                    else cerr << "Warning: Unexpected or late acknowledgement received!" << endl;
                    if(MessagesToAck.empty()) awaitingAck = false;
                    break;
                default:
                    cerr << "Warning: Packet with unknown code is received!" << endl;
                    break;
            }
        }
    }
    cout << "Press ENTER to exit..." << endl;
    bgThreadAlive = false;
    pthread_exit(0);
}

int main(int argc, char** argv) {
    struct sockaddr_in peer;
    SOCKET s;
    string username, peername, name;

    INIT();

#ifdef USE_CONNECTED_CLIENT
    s = udp_connected_client(argv[1], argv[2], &peer);
#else
    s = udp_client(argv[1], argv[2], &peer);
#endif
    chatServer = ChatServer(s, peer);

    try {
        // Log in
        cout << "Greetings! Type \"/help\" for more information." << endl;
        while (1) {
            cout << "What is your name?" << endl;
            getline(cin, username);
            if (username == "/quit") throw Exception();
            if (username == "/help") {
                showHelp();
                continue;
            }
            if (username.length() <= MAX_USERNAME_LENGTH) {
                if (username.find('#') != string::npos) {
                    cout << "Symbol # is prohibited to use in names!" << endl;
                    continue;
                } else if(username.empty()) {
                    //Empty names shall not pass!
                    continue;
                } else break;
            }
            cout << "This name is too long! It should be at most " <<
            MAX_USERNAME_LENGTH << " characters long!";
        }
        chatServer.login(username);
        if (chatServer.receiveUsersList() == 0) {
            cout << "You are successfully logged in!" << endl;
            chatServer.showUsersList();
        }

        bgThreadAlive = true;
        int r = pthread_create(&bgThread, NULL, BackgroundThread, (void*) &s);
        if(r < 0) {
            cerr << "CANNOT START THREAD" << endl;
            throw Exception();
        }
        while(bgThreadAlive) {
            /* NO SOCKET OPERATIONS HERE */
            cout << "Type name or id (with # symbol) of user you wish to chat with:" << endl;
            while (bgThreadAlive) {
                getline(cin, name);
                if (name == "/quit") throw Exception();
                if(!bgThreadAlive) break;
                if (name == "/refresh") {
                    chatServer.showUsersList();
                    continue;
                }
                if (name.length() <= MAX_USERNAME_LENGTH) {
                    peername = chatServer.startChat(name);
                    if (!peername.empty()) {
                        break;
                    } else {
                        cout << "Cannot find user \"" << name << "\"" << endl;
                        cout << "Try to type \"/refresh\" to check if this user is still online" << endl;
                    }
                }
                else
                    cout << "This name is too long! It should be at most " <<
                    MAX_USERNAME_LENGTH << " characters long!";
            }
            if(!bgThreadAlive) break;
            system("clear");
            //system("prompt [ You ]: ");
            if(peername.at(0) == '#') cout << "Public room " << peername << ":" << endl << endl;
            else cout << "Chat with " << peername << ":" << endl << endl;
            chatServer.showMessage(chatServer.getCurrentPeer());
            string str;
            while (bgThreadAlive) {
                getline(cin, str);
                if (str == "/quit") throw Exception();
                if(!bgThreadAlive) break;
                if (str == "/refresh") {
                    chatServer.showUsersList();
                    continue;
                }
                if (str == "/help") {
                    showHelp();
                    continue;
                }
                if (str == "/bye") {
                    chatServer.setCurrentPeer(0);
                    break;
                }
                {
                    SCOPE_LOCK_MUTEX(autoMutex.get());
                    MessagesToSend.push(str);
                }
                pthread_yield();
            }
            if(!bgThreadAlive) break;
            checkOnline = true;
            checkPending = true;
            //TODO find analogue of system("cls");
        }
    } catch (Exception& ex) {
        chatServer.logout();
        bgThreadAlive = false;
        pthread_join(bgThread, NULL);
    }
    cout << "Disconnecting from server..." << endl;
    shutdown(s, SHUT_RDWR);
    cout << "Closing socket and exiting..." << endl;
    CLOSE(s);
    EXIT(0);
}

void showHelp() {
    cout << "Command list:" << endl
         << "/refresh - show online users." << endl
         << "/bye - leave opened chat." << endl
         << "/quit - log out and exit." << endl
         << "/help - display this command list." << endl << endl;
}