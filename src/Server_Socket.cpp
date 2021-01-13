#include "Server_Socket.hpp"

Server_Socket::Server_Socket()
    : listen_fd(-1),
      epfd(-1)
{
    epfd = epoll_create(1024);
    if(epfd < 0)
        fprintf(stderr, "Failed to create epoll file descriptor.\n");
}

Server_Socket::~Server_Socket()
{
    if(close(listen_fd))
        fprintf(stderr, "Failed to close server listening fd, please kill it manually.");
    if(close(epfd))
        fprintf(stderr, "Failed to close epoll file descriptor.\n");
}

void Server_Socket::listen_at( const std::string ip, const int port) 
{
#ifdef _WIN32
    WSADATA wsa_data = {0};
    // Request WinSock version 2.2 
    auto result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if(result != NO_ERROR)
    {
        print_socket_error();
    }

    server_listening_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if(server_listening_socket == INVALID_SOCKET)
    {
        print_socket_error();
    }

    sockaddr_in socket_address;
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    socket_address.sin_port = htons(port);

    int bind_result = bind(
        server_listening_socket, 
        (sockaddr*)&socket_address, 
        sizeof(socket_address)
    );

    if( bind_result == INVALID_SOCKET )
    {
        print_socket_error();
    }

    listen_fd = listen( server_listening_socket, 1024 );
  
#elif __linux__
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd == -1)
    {
        perror("socket");
        return;
    }

    struct sockaddr_in ipv4_address;
    
    memset(&ipv4_address, 0 , sizeof(ipv4_address));
    
    ipv4_address.sin_family = AF_INET;
    
    if(ip == "localhost" || ip == "0.0.0.0")
        ipv4_address.sin_addr.s_addr = INADDR_ANY;
    else
        ipv4_address.sin_addr.s_addr = inet_addr(ip.c_str());
    
    ipv4_address.sin_port = htons(port);
    
    int ipv4_address_length = sizeof(ipv4_address);

    int bind_result = bind(listen_fd, (struct sockaddr*)&ipv4_address, ipv4_address_length);
    if(bind_result == -1)
    {
        // log bind error
        perror("bind");
        return;
    }

    int listen_result = listen(listen_fd, 4096);
    if(listen_result == -1)
    {
        // log bind error  
        perror("listen");
        return;
    }    
    
    printf("---Server is listening---\n");

    server_epoll_event.data.fd = listen_fd;
    server_epoll_event.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &server_epoll_event);

    constexpr int TRIGGERED_EVENTS_MAX_SIZE = 1024;

    epoll_event triggered_events[TRIGGERED_EVENTS_MAX_SIZE];
    
    int number_of_triggered_events;

    // server main loop
    for (;;)
    {
        switch(number_of_triggered_events = epoll_wait(epfd, triggered_events, TRIGGERED_EVENTS_MAX_SIZE, -1))
        {
            case -1:
                perror("epoll");
                break;

            default:
            {
                int i = 0;
                for(; i < number_of_triggered_events; ++i)
                {
                    // New client arrives
                    if((triggered_events[i].data.fd == listen_fd) && (triggered_events[i].events & EPOLLIN))
                    {
                        sockaddr_in client_socket;
                        socklen_t client_socket_length = sizeof(client_socket);

                        int client_fd = accept(listen_fd, (sockaddr*)&client_socket, &client_socket_length);
                        if(client_fd < 0)
                        {
                            perror("accept");
                            close(listen_fd);
                            return;
                        }

                        printf("Accept client: %s:%d\n", inet_ntoa(client_socket.sin_addr), htons(client_socket.sin_port));
                        
                        /**
                         * Newly added client sockets should be monitored
                         * in edge-triggered or level-triggered way?
                         */
                        server_epoll_event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
                        server_epoll_event.data.fd = client_fd;
                        epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &server_epoll_event);
                    }
                    
                    // Ready for reading
                    if((triggered_events[i].data.fd != listen_fd) && (triggered_events[i].events & EPOLLIN))
                    {
                        // It peer shudown, skip this event
                        if(triggered_events[i].events & EPOLLRDHUP)
                        {
                            epoll_ctl(epfd, EPOLL_CTL_DEL, triggered_events[i].data.fd, nullptr);
                            printf("Peer shutdown\n");
                            continue;
                        }

                        read_from(triggered_events[i].data.fd);
                        
                        fill_send_buffer("Hello client, I'm server :-)\n");

        
                        if(!write_to(triggered_events[i].data.fd, send_buffer, send_buffer.size()))
                        {
                            perror("send");
                        }
                    }
                }
            }
            break;
        }   // end of swtich()
    }   // end of for(;;)

#endif
    return;
}

bool Server_Socket::write_to(const int peer_fd, const std::vector<uint8_t>& data, const int data_size)
{
    if(!send_buffer.empty())
        send_buffer.clear();

    char local_send_buffer[8192] = { 0 };
    for(int i = 0; i < data_size; ++i)
        local_send_buffer[i] = (char)data[i];

    int send_result = send(peer_fd, local_send_buffer, data_size, 0);
    if(send_result < 0)
    {
        perror("send");
        return false;
    }

    printf("Send: %s", local_send_buffer);
    return true;
}

std::vector<uint8_t>* Server_Socket::read_from(const int peer_fd)
{
    if(!receive_buffer.empty())
        receive_buffer.clear();

    /**
     * Most web browsers set the maximum of HTTP request to 8192 bytes.
     * That's why the buffer size is magic 8192.
     * 
     * TODO: What if the data to be received exceed 8192 bytes?
     */
    char local_receive_buffer[8192] = { 0 };
    ssize_t receive_result = recv(peer_fd, &local_receive_buffer, sizeof(local_receive_buffer), 0);
    
    if(receive_result == -1)
    {
        perror("recv");
        return {};
    }

    for(int i = 0; i < receive_result; ++i)
        receive_buffer.push_back( (uint8_t)local_receive_buffer[i] );

    print_receive_buffer();

    return &receive_buffer;
}

void Server_Socket::print_socket_error()
{
#ifdef _WIN32
    wchar_t *s = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
                NULL, WSAGetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPWSTR)&s, 0, NULL);
    fprintf(stderr, "%S\n", s);
    LocalFree(s);
    WSACleanup();
#elif __linux__

#endif
}

std::vector<uint8_t>* Server_Socket::get_receive_buffer()
{
    return &receive_buffer;
}

std::vector<uint8_t>* Server_Socket::get_send_buffer()
{
    return &send_buffer;
}

void Server_Socket::print_receive_buffer()
{
    std::string receive_buffer_string;
    for(const auto& byte : receive_buffer)
        receive_buffer_string += (char)byte;
    
    printf("Receive: %s\n", receive_buffer_string.c_str());
}

void Server_Socket::fill_send_buffer(const std::string& data_string)
{
    if(!send_buffer.empty())
        send_buffer.clear();

    for(int i = 0; i < data_string.size(); ++i)
        send_buffer.push_back( data_string[i] );
}
    
void Server_Socket::fill_send_buffer(const std::vector<uint8_t>& data_stream)
{   
    send_buffer = data_stream;
}
