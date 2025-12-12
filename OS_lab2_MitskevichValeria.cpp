#include <iostream>
#include <cstring>
#include <csignal>
#include <cerrno>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>

#define PORT 5050
#define BUF_SIZE 1024

using namespace std;


volatile sig_atomic_t wasSigHup = 0;
int client_socket = -1;

void sigHupHandler(int sig) {
    wasSigHup = 1;
}

int main() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigHupHandler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sa, nullptr);

    int serv_socket;
    struct sockaddr_in server_addr {};
    socklen_t client_len = sizeof(struct sockaddr_in);

    serv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_socket == -1) {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(serv_socket,
        reinterpret_cast<struct sockaddr*>(&server_addr),
        sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(serv_socket);
        return 1;
    }

    if (listen(serv_socket, SOMAXCONN) == -1) {
        perror("listen");
        close(serv_socket);
        return 1;
    }

    cout << "Сервер запущен и слушает порт " << PORT << endl;

    fd_set fds;
    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);

    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    while (true) {

        if (wasSigHup) {
            cout << "Получен сигнал SIGHUP." << endl;
            if (client_socket != -1) {
                close(client_socket);
                client_socket = -1;
                cout << "Сокет клиента закрыт." << endl;
            }
            wasSigHup = 0;
            continue;
        }

        FD_ZERO(&fds);
        FD_SET(serv_socket, &fds);
        if (client_socket != -1) {
            FD_SET(client_socket, &fds);
        }

        int max_fd = (client_socket > serv_socket) ? client_socket : serv_socket;

        int res = pselect(max_fd + 1, &fds, nullptr, nullptr, nullptr, &origMask);

        if (res == -1) {
            if (errno == EINTR) {
                cout << "pselect был прерван сигналом." << endl;
                continue;
            }
            else {
                perror("pselect");
                break;
            }
        }

        // Новое соединение
        if (FD_ISSET(serv_socket, &fds)) {
            int new_client_socket = accept(serv_socket, nullptr, &client_len);
            cout << "Принято новое соединение." << endl;

            if (new_client_socket == -1) {
                perror("accept");
            }
            else {
                if (client_socket == -1) {
                    client_socket = new_client_socket;
                }
                else {
                    close(new_client_socket);
                    cout << "Закрыто лишнее соединение." << endl;
                }
            }
        }

        if (client_socket != -1 && FD_ISSET(client_socket, &fds)) {
            char buf[BUF_SIZE];
            ssize_t size_data = read(client_socket, buf, BUF_SIZE);

            if (size_data > 0) {
                cout << "Получено " << size_data << " байтов данных." << endl;
            }
            else if (size_data == 0) {
                cout << "Клиент закрыл соединение." << endl;
                close(client_socket);
                client_socket = -1;
            }
            else {
                perror("Ошибка получения данных");
            }
        }
    }

    close(serv_socket);
    if (client_socket != -1) close(client_socket);
    cout << "Сокет сервера закрыт." << endl;

    return 0;
}
