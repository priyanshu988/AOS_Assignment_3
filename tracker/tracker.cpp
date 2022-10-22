#include <bits/stdc++.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>
#include <fstream>

using namespace std;

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

struct fileData
{
    string portno;
    string file_path;
    string ip;
    string hash;
    string gid;
    bool status;
    string userid;
    long int fsize;
};
struct groupFileData
{
    string uid;
    string fname;
    string ip;
    string port;
    bool status;
};

struct group
{
    string admin;
};

struct login
{
    bool status = false;
};

map<string, group> group_ID_map;
map<string, login> login_status;
map<string, int> group_ID_status;
map<string, vector<string>> group_users; // group members
map<string, vector<string>> group_join_requests;
map<string, string> user_records;
map<string, vector<fileData>> file_records;           // file records filename->filedata
map<string, vector<groupFileData>> g_file_records;    // file records in a group gid->groupfiledata
map<string, vector<pair<string, string>>> user_files; // map userid to to pair of grpid and filename
vector<string> entry;
pthread_t client_name;
vector<string> split_string(string str, char delim);

void *server_client(void *socket_num)
{
    int conn_sock = *((int *)socket_num);

    while (true)
    {
        char buffer[1024] = {0};
        int status = read(conn_sock, buffer, 1024);
        if (status == 0)
            return socket_num;

        string buffer_str = string(buffer);
        entry = split_string(buffer_str, ',');

        if (entry[0] == "login")
        {

            int n = 0;
            if (!login_status[entry[1]].status)
            {
                if (user_records[entry[1]] == entry[2])
                {
                    n = 1;
                    login_status[entry[1]].status = true;
                    vector<pair<string, string>> v1 = user_files[entry[1]];
                    if (v1.size() != 0)
                    {
                        for (int i = 0; i < int(v1.size()); i++)
                        {
                            pair<string, string> p = v1[i];
                            string gid = p.first;
                            string fname = p.second;

                            vector<fileData> file_vec = file_records[fname];

                            for (auto j : file_vec)
                            {
                                if (j.userid == entry[1] && j.gid == gid)
                                {
                                    j.status = true;
                                }
                            }
                            vector<groupFileData> g_file_vec = g_file_records[gid];
                            for (auto j : g_file_vec)
                            {
                                if (j.uid == entry[1])
                                {
                                    j.status = true;
                                }
                            }
                        }
                    }
                }
            }
            else
                n = 2;
            send(conn_sock, &n, sizeof(n), 0);
        }
        else if (entry[0] == "create_user")
        {
            int n = 0;
            user_records[entry[1]] = entry[2];

            ofstream outf;
            outf.open("user_pass.txt", ios::app | ios::out);
            outf << entry[1] << "," << entry[2] << endl;
            outf.close();
            n = 1;
            send(conn_sock, &n, sizeof(n), 0);
        }
        else if (entry[0] == "create_group")
        {
            group *G = new group();
            G->admin = entry[2];
            group_ID_map[entry[1]] = *G;
            group_ID_status[entry[1]] = 1;
            vector<string> v;
            v.push_back(entry[2]);
            group_users[entry[1]] = v;
            int n = 1;
            send(conn_sock, &n, sizeof(n), 0);
        }
        else if (entry[0] == "list_groups")
        {
            string data_to_send = "";
            int i = 0;
            for (auto m : group_ID_map)
            {
                if (i == 0)
                {
                    data_to_send = data_to_send + m.first;
                    i = 1;
                }
                else
                {
                    data_to_send = data_to_send + "," + m.first;
                }
            }
            char *data_in_charstar = new char[data_to_send.length() + 1];
            strcpy(data_in_charstar, data_to_send.c_str());
            send(conn_sock, data_in_charstar, strlen(data_in_charstar), 0);
        }
        else if (entry[0] == "leave_group")
        {
            string data_to_send = "";
            if (group_ID_status[entry[1]] != 1)
            {
                data_to_send = "[-]Group ID is Invalid";
            }
            else
            {
                vector<string> v = group_users[entry[1]];
                int f = 0;
                for (int i = 0; i < int(v.size()); i++)
                {
                    if (v[i] == entry[2])
                    {
                        data_to_send = "[+]Successfully Left the Group";
                        v.erase(v.begin() + i);
                        group_users[entry[1]] = v;
                        f = 1;
                        break;
                    }
                }
                if (f == 1)
                {
                    data_to_send = "[-]Not a part of this group";
                }
            }
            char *data_in_charstar = new char[data_to_send.length() + 1];
            strcpy(data_in_charstar, data_to_send.c_str());
            send(conn_sock, data_in_charstar, strlen(data_in_charstar), 0);
        }
        else if (entry[0] == "join_group")
        {
            if (group_ID_status[entry[1]] != 1)
            {
                int n = 0;
                send(conn_sock, &n, sizeof(n), 0);
            }
            vector<string> v;
            v = group_join_requests[entry[1]];
            v.push_back(entry[2]);
            group_join_requests[entry[1]] = v;
            int n = 1;
            send(conn_sock, &n, sizeof(n), 0);
        }
        else if (entry[0] == "list_requests")
        {
            if (group_ID_map[entry[1]].admin != entry[2])
            {
                string data_to_send = "NOT ALLOWED";
                char *data_in_charstar = new char[data_to_send.length() + 1];
                strcpy(data_in_charstar, data_to_send.c_str());
                send(conn_sock, data_in_charstar, strlen(data_in_charstar), 0);
            }
            else
            {
                string data_to_send = "";
                vector<string> m = group_join_requests[entry[1]];
                for (int i = 0; i < int(m.size()); i++)
                {
                    if (i == 0)
                    {
                        data_to_send = data_to_send + m[i];
                    }
                    else
                    {
                        data_to_send = data_to_send + "," + m[i];
                    }
                }
                char *data_in_charstar = new char[data_to_send.length() + 1];
                strcpy(data_in_charstar, data_to_send.c_str());
                send(conn_sock, data_in_charstar, strlen(data_in_charstar), 0);
            }
        }
        else if (entry[0] == "accept_request")
        {
            if (group_ID_map[entry[1]].admin != entry[2])
            {
                int n = 0;
                send(conn_sock, &n, sizeof(n), 0);
            }
            else
            {
                vector<string> m = group_join_requests[entry[1]];
                vector<string> v = group_users[entry[1]];
                for (int i = 0; i < int(m.size()); i++)
                {
                    if (m[i] == entry[3])
                    {
                        m.erase(m.begin() + i);
                        group_join_requests[entry[1]] = m;
                        v.push_back(entry[3]);
                        group_users[entry[1]] = v;
                        break;
                    }
                }
                int n = 1;
                send(conn_sock, &n, sizeof(n), 0);
            }
        }
        else if (entry[0] == "upload_file")
        {
            // check if the user is in group or not
            // if no then return not a part of group
            // if yes then create SHA1 hash
            // then create the file data struct to store the details of torrent file
            // map that torrent file data to group id

            string data_to_send;
            bool flag1 = false;
            for (auto m : group_ID_map)
            {
                if (entry[1] == m.first)
                    flag1 = true;
            }

            if (!flag1)
            {
                data_to_send = "[-] Invalid Group ID";
            }
            else
            {
                bool flag2 = false;
                vector<string> v = group_users[entry[1]];
                for (auto i : v)
                {
                    if (entry[2] == i)
                        flag2 = true;
                }

                if (!flag2)
                {
                    data_to_send = "[-] Not a Member of This Group";
                }
                else
                {
                    fileData *fdata = new fileData();
                    groupFileData *gdata = new groupFileData();
                    fdata->fsize = stoi(entry[6]);
                    fdata->file_path = entry[3];
                    fdata->gid = entry[1];
                    fdata->hash = entry[7];
                    fdata->ip = entry[4];
                    fdata->portno = entry[5];
                    fdata->status = true;
                    fdata->userid = entry[2];
                    gdata->fname = getFileName(entry[3]);
                    gdata->ip = entry[4];
                    gdata->port = entry[5];
                    gdata->uid = entry[2];
                    gdata->status = true;
                    string file_name = getFileName(entry[3]);
                    vector<pair<string, string>> v1 = user_files[entry[2]];
                    vector<fileData> file_vec = file_records[file_name];
                    pair<string, string> p;
                    v1.push_back(p);
                    p.first = entry[1];
                    p.second = getFileName(entry[3]);
                    if (file_vec.size() == 0)
                    {
                        file_records[file_name].push_back(*fdata);
                        g_file_records[entry[1]].push_back(*gdata);
                        data_to_send = "[+] Successfully Uploaded";
                        user_files[entry[2]] = v1;
                    }
                    else
                    {
                        bool flag3 = false;
                        for (auto i : file_vec)
                        {
                            if (i.userid == entry[2] && i.gid == entry[1])
                            {
                                data_to_send = "[+] Already Uploaded";
                                flag3 = true;
                                break;
                            }
                        }
                        if (!flag3)
                        {
                            file_records[file_name].push_back(*fdata);
                            g_file_records[entry[1]].push_back(*gdata);
                            data_to_send = "[+] Successfully Uploaded";
                            user_files[entry[2]] = v1;
                        }
                    }
                }
            }
            // send msg to client
            char *data_in_charstar = new char[data_to_send.length() + 1];
            strcpy(data_in_charstar, data_to_send.c_str());
            send(conn_sock, data_in_charstar, strlen(data_in_charstar), 0);
        }
        else if (entry[0] == "download_file")
        {
            string data_to_send;
            bool flag1 = false;
            for (auto m : group_ID_map)
            {
                if (entry[1] == m.first)
                    flag1 = true;
            }
            if (!flag1)
            {
                data_to_send = "[-] Invalid Group ID";
            }
            else
            {
                bool flag2 = false;
                vector<string> v = group_users[entry[1]];
                for (auto i : v)
                {
                    if (entry[2] == i)
                        flag2 = true;
                }

                if (!flag2)
                {
                    data_to_send = "[-] Not a Member of This Group";
                }
                else
                {
                    vector<groupFileData> v = g_file_records[entry[1]];
                    int flag3 = 0;
                    for (auto i : v)
                    {
                        if (i.fname == entry[3] && i.status)
                        {
                            flag3 = 1;
                            break;
                        }
                    }
                    if (flag3 == 0)
                    {
                        data_to_send = "[-] File not Seeded in this Group or not available to share at this moment";
                    }
                    else
                    {
                        vector<fileData> v1 = file_records[entry[3]];
                        int ind = 0;
                        for (auto i : v1)
                        {
                            string s = "";
                            if (i.gid == entry[1] && i.status)
                            {
                                s = i.ip + ':' + i.portno;
                            }
                            if (ind == 0)
                            {
                                data_to_send = s;
                                ind++;
                            }
                            else
                            {
                                data_to_send = ',' + s;
                            }
                        }
                    }
                }
            }
            char *data_in_charstar = new char[data_to_send.length() + 1];
            strcpy(data_in_charstar, data_to_send.c_str());
            send(conn_sock, data_in_charstar, strlen(data_in_charstar), 0);
        }
        else if (entry[0] == "list_files")
        {
            string data_to_send = "";
            bool flag1 = false;
            for (auto m : group_ID_map)
            {
                if (entry[1] == m.first)
                    flag1 = true;
            }

            if (!flag1)
            {
                data_to_send = "[-] Invalid Group ID";
            }
            else
            {
                bool flag2 = false;
                vector<string> v = group_users[entry[1]];
                for (auto i : v)
                {
                    if (entry[2] == i)
                        flag2 = true;
                }

                if (!flag2)
                {
                    data_to_send = "[-] Not a Member of This Group";
                }
                else
                {
                    vector<groupFileData> v = g_file_records[entry[1]];
                    if (v.size() == 0)
                    {
                        data_to_send = "[-] No Files to Share";
                    }
                    else
                    {
                        int ind = 0;
                        for (auto i : v)
                        {
                            if (ind == 0)
                            {
                                data_to_send = data_to_send + i.fname;
                                ind++;
                            }
                            else
                            {
                                data_to_send = data_to_send + "," + i.fname;
                            }
                        }
                    }
                }
                char *data_in_charstar = new char[data_to_send.length() + 1];
                strcpy(data_in_charstar, data_to_send.c_str());
                send(conn_sock, data_in_charstar, strlen(data_in_charstar), 0);
            }
        }
        else if (entry[0] == "logout")
        {

            int n = 0;
            if (login_status[entry[1]].status)
            {
                n = 1;
                login_status[entry[1]].status = false;
                vector<pair<string, string>> v1 = user_files[entry[1]];
                if (v1.size() != 0)
                {
                    for (int i = 0; i < int(v1.size()); i++)
                    {
                        pair<string, string> p = v1[i];
                        string gid = p.first;
                        string fname = p.second;

                        vector<fileData> file_vec = file_records[fname];

                        for (auto j : file_vec)
                        {
                            if (j.userid == entry[1] && j.gid == gid)
                            {
                                j.status = false;
                            }
                        }
                        vector<groupFileData> g_file_vec = g_file_records[gid];
                        for (auto j : g_file_vec)
                        {
                            if (j.uid == entry[1])
                            {
                                j.status = false;
                            }
                        }
                    }
                }
            }

            send(conn_sock, &n, sizeof(n), 0);
        }
    }

    return socket_num;
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

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        perror("[-]Invalid Arguments");
        exit(1);
    }

    ifstream inp_file("user_pass.txt");
    string line;
    while (getline(inp_file, line))
    {
        entry = split_string(line, ',');
        user_records[entry[0]] = entry[1];
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("[-]error in socket");
        exit(1);
    }
    printf("[+]Server Socket Created\n");

    const char *ipaddress = "127.0.0.1";
    int port = 8080;
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ipaddress);
    int addrlen = sizeof(sockaddr);
    int e = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (e < 0)
    {
        perror("[-]Error in bind");
        exit(1);
    }
    printf("[+]Binding successfull.\n");

    if (listen(sockfd, 10) == 0)
    {
        printf("[+]Listening....\n");
    }
    else
    {
        perror("[-]Error in listening");
        exit(1);
    }
    int new_sock;

    while (1)
    {
        if ((new_sock = accept(sockfd, (struct sockaddr *)&server_addr, (socklen_t *)&addrlen)) < 0)
        {
            cout << "error in connection establishment" << endl;

            exit(1);
        }

        int client_port_num = ntohs(server_addr.sin_port);
        string client_port_str = to_string(client_port_num);
        int *argument = (int *)malloc(sizeof(*argument));
        *argument = new_sock;

        if (pthread_create(&client_name, 0, server_client, argument) < 0)
        {
            cout << "could not create thread" << endl;

            return 1;
        }
    }
}