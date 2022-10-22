#include <bits/stdc++.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>
#include <fstream>

using namespace std;
size_t getfile_size(string filename)
{
    struct stat stat_obj;
    long long res = stat(filename.c_str(), &stat_obj);
    if (res == -1)
        return res;
    else
        return stat_obj.st_size;
}
string getFileName(string path)
{
    int n = path.size();
    int s = n - 1;
    while (path[s--] != '/')
        ;
    s += 2;
    string ans = "";
    while (s < n)
    {
        ans += path[s++];
    }
    return ans;
}
map<string, string> file_path_record;   // key->filename value ->filepath
map<string, vector<int>> chunk_records; // map: key = filename, value = chunk status array

int num_chunks = 0;

string convert_into_hash(string file_path)
{
    num_chunks = 0;
    struct stat stat_obj;
    long long res = stat(file_path.c_str(), &stat_obj);
    if (res)
    {
    }
    long long fsize = stat_obj.st_size;

    size_t req_size = 512 * 1024;

    string hashstr = "";

    ifstream inp_file;
    inp_file.open(file_path, ios::binary | ios::in);
    if (fsize < long(req_size))
    {
        req_size = fsize;
    }
    string file_name = getFileName(file_path);
    file_path_record[file_name] = file_path;
    char temporary[req_size];

    while (inp_file.read(temporary, req_size))
    {
        unsigned char md[20];
        num_chunks++;
        if (SHA1(reinterpret_cast<const unsigned char *>(temporary), string(temporary).length(), md))
        {
            for (int i = 0; i < 20; i++)
            {
                char buf[3];
                sprintf(buf, "%02x", md[i] & 0xff);
                hashstr += string(buf);
            }
        }

        fsize = fsize - req_size;
        // string sha_chunk = sha1(temporary, req_size);
        // hashstr.append(sha_chunk.substr(0, 20));
        if (fsize == 0)
            break;
        if (fsize < long(req_size))
            req_size = fsize;
    }
    vector<int> v(num_chunks, 1);
    chunk_records[file_name] = v;

    return hashstr;
}

vector<string> split_string(string str, char delim)
{
    std::vector<string> v;
    int i = 0;
    string s = "";
    while (i < int(str.length()))
    {
        if (str[i] != delim)
        {
            s = s + str[i];
            i++;
        }
        else
        {
            v.push_back(s);
            s = "";
            i++;
        }
    }
    v.push_back(s);
    return v;
}
void write_file(int sockfd, string filepath)
{
    int n;
    FILE *fp;
    const char *filename = filepath.c_str();
    char buffer[1024];

    fp = fopen(filename, "w");
    while (1)
    {
        n = recv(sockfd, buffer, 1024, 0);
        if (n <= 0)
        {
            break;
            return;
        }
        fprintf(fp, "%s", buffer);
        bzero(buffer, 1024);
    }
    return;
}

void peerClient(vector<string> ipport, string dst, string fileName)
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cout << "[-] Socket creation error" << endl;
        return;
    }
    map<int, vector<string>> mp;
    for (auto i : ipport)
    {
        vector<string> temp = split_string(i, ':');
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(stoi(temp[1]));

        if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
        {
            cout << "[-] Invalid address/ Address not supported" << endl;
            return;
        }

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            cout << "Connection Failed" << endl;
            return;
        }
        cout << "[+] Connected" << endl;

        string data_to_send = fileName;
        char *data_in_charstar = new char[data_to_send.length() + 1];
        std::strcpy(data_in_charstar, data_to_send.c_str());
        send(sock, data_in_charstar, strlen(data_in_charstar), 0);
        char chunkstat[1024] = {0};
        int status = read(sock, chunkstat, 1024);
        if (status)
        {
        }
        string chunkstat_str = string(chunkstat);

        for (int ind = 0; ind < int(chunkstat_str.length()); ind++)
        {
            int check = chunkstat_str[ind] - '0';
            if (check == 1)
            {
                vector<string> tempo = mp[ind];
                tempo.push_back(temp[1]);
                mp[ind] = tempo;
            }
        }
    }
    string reply1 = to_string(1);
    char *reply1_str = new char[reply1.length() + 1];
    std::strcpy(reply1_str, reply1.c_str());
    send(sock, reply1_str, strlen(reply1_str), 0);

    string filePath = dst + fileName;
    write_file(sock, filePath);
    printf("[+]Data written in the file successfully.\n");
}
void send_file(FILE *fp, int sockfd)
{

    char data[1024] = {0};

    while (fgets(data, 1024, fp) != NULL)
    {
        if (send(sockfd, data, sizeof(data), 0) == -1)
        {
            perror("[-]Error in sending file.");
            exit(1);
        }
        bzero(data, 1024);
    }
}

void *peerServer(void *clientIP)
{
    int serverfd;
    struct sockaddr_in address;
    int opt = 1, newSock;
    int addrlen = sizeof(address);
    string c_ip = *(string *)clientIP;
    string client_ip, sport;
    char *s_ip;
    vector<string> ipport = split_string(c_ip, ':');
    client_ip = ipport[0];
    sport = ipport[1];
    s_ip = new char[client_ip.length()];
    std::strcpy(s_ip, client_ip.c_str());
    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("[-] Socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("[-] Setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_port = htons(stoi(sport));

    if (inet_pton(AF_INET, s_ip, &address.sin_addr) <= 0)
    {
        cout << "[-] Invalid address/ Address not supported" << endl;
        return clientIP;
    }

    if (bind(serverfd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("[-] Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(serverfd, 10) < 0)
    {
        perror("[-] Not Listening");
        exit(EXIT_FAILURE);
    }
    cout << "[+] Server Connected" << endl;

    while (1)
    {
        if ((newSock = accept(serverfd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            exit(EXIT_FAILURE);
        }
        else
        {
            // cout << "Waiting...." << endl;
            char buffer[1024] = {0};
            int connSock = newSock;
            int status = read(connSock, buffer, 1024);
            if (status == 0)
                return clientIP;
            string fileName = string(buffer);
            FILE *fp;

            fp = fopen(fileName.c_str(), "r");
            if (fp == NULL)
            {
                perror("[-]Error in reading file.");
                exit(1);
            }

            send_file(fp, connSock);
            printf("[+]File data sent successfully.\n");

            close(connSock);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        perror("Not a Valid Argument");
        exit(1);
    }
    string clientIP = string(argv[1]);
    pthread_t serverthread;
    pthread_attr_t thread_attr2;
    int res2 = pthread_attr_init(&thread_attr2);
    if (res2 != 0)
    {
        perror("[-] Attribute creation failed");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&serverthread, &thread_attr2, peerServer, (void *)&clientIP) < 0)
    {
        perror("could not create thread");
        return 1;
    }

    struct sockaddr_in server_addr;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("[-]error in socket creation");
        exit(1);
    }
    printf("[+]Server socket created successfully.\n");

    vector<string> ipport = split_string(argv[1], ':');
    char *peer_ip = new char[ipport[0].length() + 1];
    std::strcpy(peer_ip, ipport[0].c_str());

    int peer_port = stoi(ipport[1]);

    const char *ip = "127.0.0.1";
    int port = 8080;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    int e = connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (e == -1)
    {
        perror("[-]Error in socket");
        exit(1);
    }
    printf("[+]Connected to Server.\n");
    bool loginFlag = false;
    string opt, user_id, password;
    while (1)
    {

        cin >> opt;
        if (opt == "login")
        {
            cin >> user_id >> password;
            if (!loginFlag)
            {
                int flag_validation;
                string data_to_send = opt + "," + user_id + "," + password;
                char *data_in_charstar = new char[data_to_send.length() + 1];
                std::strcpy(data_in_charstar, data_to_send.c_str());

                send(sock_fd, data_in_charstar, strlen(data_in_charstar), 0);
                recv(sock_fd, &flag_validation, sizeof(flag_validation), 0);

                if (flag_validation != 1)
                {
                    if (flag_validation != 2)
                    {
                        cout << "[-]Wrong UserID and Password" << endl;
                        user_id = "";
                        password = "";
                    }
                    else
                        cout << "[-]Already Having an Active Session" << endl;
                }
                else
                {

                    cout << "[+]Successfully Logged In" << endl;
                    loginFlag = true;
                }
            }
            else
            {
                cout << "[-]Logout First" << endl;
            }
        }
        else if (opt == "leave_group")
        {
            string group_id;
            cin >> group_id;
            if (loginFlag)
            {
                string data_to_send = opt + "," + group_id + "," + user_id;
                char *data_in_charstar = new char[data_to_send.length() + 1];
                std::strcpy(data_in_charstar, data_to_send.c_str());
                send(sock_fd, data_in_charstar, strlen(data_in_charstar), 0);
                char buffer[1024] = {0};
                int status = read(sock_fd, buffer, 1024);
                if (status)
                {
                }
                string buffer_str = string(buffer);
                cout << buffer_str << endl;
            }
            else
            {
                cout << "[-]Login First" << endl;
            }
        }
        else if (opt == "create_user")
        {
            int validbit;
            cin >> user_id >> password;
            string del = ",";
            string data = opt + del + user_id + del + password;
            if (!loginFlag)
            {
                char *data_char = new char[data.length() + 1];
                std::strcpy(data_char, data.c_str());
                send(sock_fd, data_char, strlen(data_char), 0);
                recv(sock_fd, &validbit, sizeof(validbit), 0);
                if (validbit == 1)
                {

                    cout << "[+]user created successfully " << endl;
                }
                else
                {
                    cout << "[-]unsuccessful creation : You are already logged in" << endl;
                }
            }

            else
            {
                cout << "[-]Logout First" << endl;
            }
        }
        else if (opt == "create_group")
        {
            string group_id;
            cin >> group_id;
            string data_to_send = opt + "," + group_id + "," + user_id;
            if (loginFlag)
            {
                char *data_in_charstar = new char[data_to_send.length() + 1];
                std::strcpy(data_in_charstar, data_to_send.c_str());
                int check;
                send(sock_fd, data_in_charstar, strlen(data_in_charstar), 0);
                recv(sock_fd, &check, sizeof(check), 0);

                if (check != 1)
                {
                    cout << "[-]Group Not Created" << endl;
                }
                else
                {

                    cout << "[+]Successfully Created Group: " << group_id << endl;
                }
            }
            else
            {
                cout << "[-]Login First" << endl;
            }
        }
        else if (opt == "list_groups")
        {
            if (loginFlag)
            {
                string data_to_send = opt;
                char *data_in_charstar = new char[data_to_send.length() + 1];
                std::strcpy(data_in_charstar, data_to_send.c_str());
                char buffer[1024] = {0};
                send(sock_fd, data_in_charstar, strlen(data_in_charstar), 0);
                int status = read(sock_fd, buffer, 1024);
                if (status)
                {
                }
                vector<string> list_groups;
                string buffer_str = string(buffer);
                list_groups = split_string(buffer_str, ',');
                // list_groups = split_string(string(list_of_groups),',');
                // cout << buffer_str <<endl;
                for (int i = 0; i < int(list_groups.size()); i++)
                {
                    cout << "[" << i + 1 << "] " << list_groups[i] << endl;
                }
            }
            else
            {
                cout << "[-]Login First" << endl;
            }
        }
        else if (opt == "join_group")
        {
            string group_id;
            cin >> group_id;
            if (loginFlag)
            {
                string data_to_send = opt + "," + group_id + "," + user_id;
                char *data_in_charstar = new char[data_to_send.length() + 1];
                std::strcpy(data_in_charstar, data_to_send.c_str());
                int check;
                send(sock_fd, data_in_charstar, strlen(data_in_charstar), 0);
                recv(sock_fd, &check, sizeof(check), 0);
                if (check != 1)
                {
                    cout << "[-]Error in sending Join Request";
                }
                else
                {

                    cout << "[+]Successfully Sent Joining Request to Group: " << group_id << endl;
                }
            }
            else
            {
                cout << "[-]Login First" << endl;
            }
        }
        else if (opt == "list_requests")
        {
            string group_id;
            cin >> group_id;
            if (loginFlag)
            {
                string data_to_send = opt + "," + group_id + "," + user_id;
                char *data_in_charstar = new char[data_to_send.length() + 1];
                std::strcpy(data_in_charstar, data_to_send.c_str());
                send(sock_fd, data_in_charstar, strlen(data_in_charstar), 0);
                char buffer[1024] = {0};

                int status = read(sock_fd, buffer, 1024);
                if (status)
                {
                }
                vector<string> list_requests;
                string buffer_str = string(buffer);
                list_requests = split_string(buffer_str, ',');
                for (int i = 0; i < int(list_requests.size()); i++)
                {
                    cout << list_requests[i] << endl;
                }
            }
            else
            {
                cout << "[-]Login First" << endl;
            }
        }
        else if (opt == "accept_request")
        {
            string group_id, user_id2;
            cin >> group_id >> user_id2;
            if (loginFlag)
            {
                string data_to_send = opt + "," + group_id + "," + user_id + ',' + user_id2;
                char *data_in_charstar = new char[data_to_send.length() + 1];
                std::strcpy(data_in_charstar, data_to_send.c_str());
                send(sock_fd, data_in_charstar, strlen(data_in_charstar), 0);
                int check;
                recv(sock_fd, &check, sizeof(check), 0);

                if (check != 0)
                {
                    cout << "[+]Requested Accepted" << endl;
                }
                else
                {
                    cout << "[-]Not Valid Arguments" << endl;
                }
            }
            else
            {
                cout << "[-]Login First" << endl;
            }
        }
        else if (opt == "upload_file")
        {
            string group_id, file_path;
            cin >> file_path >> group_id;
            if (loginFlag)
            {
                struct stat stat_obj;
                long long res = stat(file_path.c_str(), &stat_obj);
                if (res)
                {
                }
                long long fsize = stat_obj.st_size;
                string hashstr = convert_into_hash(file_path);

                string data_to_send = opt + "," + group_id + "," + user_id + ',' + file_path + ',' + peer_ip + ',' + to_string(peer_port) + ',' + to_string(fsize) + ',' + hashstr + ',' + to_string(num_chunks);
                char *data_in_charstar = new char[data_to_send.length() + 1];
                std::strcpy(data_in_charstar, data_to_send.c_str());
                send(sock_fd, data_in_charstar, strlen(data_in_charstar), 0);

                char buffer[1024] = {0};
                int status = read(sock_fd, buffer, 1024);
                if (status)
                {
                }
                string buffer_str = string(buffer);
                cout << buffer_str << endl;
            }
            else
            {
                cout << "[-]Login First" << endl;
            }
        }
        else if (opt == "download_file")
        {
            string group_id, file_name, dest;
            cin >> group_id >> file_name >> dest;
            if (loginFlag)
            {
                string data_to_send = opt + "," + group_id + "," + user_id + ',' + file_name;
                char *data_in_charstar = new char[data_to_send.length() + 1];
                std::strcpy(data_in_charstar, data_to_send.c_str());
                send(sock_fd, data_in_charstar, strlen(data_in_charstar), 0);
                char buffer[4096] = {0};
                int status = read(sock_fd, buffer, 4096);
                if (status)
                {
                }
                string buffer_str = string(buffer);
                if (buffer_str == "[-] Invalid Group ID" || buffer_str == "[-] Not a Member of This Group" || buffer_str == "[-] File not Seeded in this Group")
                    cout << buffer_str << endl;
                else
                {
                    vector<string> ip_ports = split_string(buffer_str, ',');
                    cout << "";
                    peerClient(ip_ports, dest, file_name);
                }
            }
            else
            {
                cout << "[-]Login First" << endl;
            }
        }
        else if (opt == "list_files")
        {
            string groupid;
            cin >> groupid;
            if (loginFlag)
            {
                string data_to_send = opt + "," + groupid + "," + user_id;
                char *data_in_charstar = new char[data_to_send.length() + 1];
                std::strcpy(data_in_charstar, data_to_send.c_str());
                send(sock_fd, data_in_charstar, strlen(data_in_charstar), 0);
                char buffer[1024] = {0};

                int status = read(sock_fd, buffer, 1024);
                if (status)
                {
                }
                vector<string> list_requests;
                string buffer_str = string(buffer);
                list_requests = split_string(buffer_str, ',');
                for (int i = 0; i < int(list_requests.size()); i++)
                {
                    cout << list_requests[i] << endl;
                }
            }
            else
            {
                cout << "[-]Login First" << endl;
            }
        }
        else if (opt == "logout")
        {
            if (loginFlag)
            {
                int flag_validation = 0;
                string data_to_send = opt + ',' + user_id;
                char *data_in_charstar = new char[data_to_send.length() + 1];
                std::strcpy(data_in_charstar, data_to_send.c_str());
                send(sock_fd, data_in_charstar, strlen(data_in_charstar), 0);
                recv(sock_fd, &flag_validation, sizeof(flag_validation), 0);

                if (flag_validation != 1)
                {
                    cout << "[+]Unsuccessful" << endl;
                }
                else
                {

                    cout << "[+]Successfully Logged Out" << endl;
                    loginFlag = false;
                    user_id = "";
                    password = "";
                }
            }
            else
            {
                cout << "[-]Not Logged in Yet" << endl;
            }
        }
    }
}