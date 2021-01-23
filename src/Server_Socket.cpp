#include "Server_Socket.hpp"

namespace
{
    /**
     * Epoll triggered events max size
     */
    constexpr size_t TRIGGERED_EVENTS_MAX_SIZE = 1024;

    /**
     * Epoll interest/event list size
     */
    constexpr size_t EPOLL_INTEREST_LIST_SIZE = 1024;

    /**
     * Maximum socket listening buffer size in byte
     */
    constexpr size_t MAXIMUM_LISTENING_PENDING_QUEUE = 4096;

    /**
     * Maximum sending/receiving buffer size in byte
     */
    constexpr size_t MAXIMUM_BUFFER_SIZE = 8192;
}

Server_Socket::Server_Socket()
    : server_socket_state(Server_Socket_State::UNINITIALIZED),
      listen_fd(-1),
      epfd(-1),
      triggered_events(new epoll_event()),
      has_finished_initialization(false),
      readable_fd(-1)
{
    epfd = epoll_create(EPOLL_INTEREST_LIST_SIZE);
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

bool Server_Socket::initialize_server_socket(const std::string ip, const int port)
{
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd == -1)
    {
        // TODO: log it
        perror("socket");
        return false;
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
        // TODO: log it
        perror("bind");
        return false;
    }

    int listen_result = listen(listen_fd, MAXIMUM_LISTENING_PENDING_QUEUE);
    if(listen_result == -1)
    {
        // TODO: log it
        perror("listen");
        return false;
    }
    
    printf("---Server is listening---\n");

    epoll_event epoll_event_helper;
    epoll_event_helper.data.fd = listen_fd;
    epoll_event_helper.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &epoll_event_helper);
    
    has_finished_initialization = true;
    return true;
}

Server_Socket_State Server_Socket::listen_at(const std::string ip, const int port)
{
    if(!has_finished_initialization)
        initialize_server_socket(ip, port);

    int number_of_triggered_events;
    for (;;)
    {
        switch(number_of_triggered_events = epoll_wait(epfd, triggered_events, TRIGGERED_EVENTS_MAX_SIZE, -1))
        {
            case -1:
            {
                // TODO: log it
                perror("epoll");
                return Server_Socket_State::ERROR;
            }
            
            default:
            {
                int i = 0;
                for(; i < number_of_triggered_events; ++i)
                {
                    int triggered_fd = triggered_events[i].data.fd;
                    uint32_t triggered_event = triggered_events[i].events;

                    // New client arrives
                    if((triggered_fd == listen_fd) && (triggered_event & EPOLLIN))
                    {
                        sockaddr_in client_socket;
                        socklen_t client_socket_length = sizeof(client_socket);

                        int client_fd = accept(listen_fd, (sockaddr*)&client_socket, &client_socket_length);
                        if(client_fd < 0)
                        {
                            // TODO: log it
                            perror("accept");
                            close(listen_fd);
                            return Server_Socket_State::ERROR;
                        }

                        printf("Accept client: %s:%d\n", inet_ntoa(client_socket.sin_addr), htons(client_socket.sin_port));

                        if(!set_socket_non_blocking(client_fd))
                        {
                            // log: can not set client socket to non-blocking
                            printf("can not set socket to non-blocking\n");
                            continue;
                        }
                        else
                        {
                            epoll_event epoll_event_helper;
                            epoll_event_helper.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
                            epoll_event_helper.data.fd = client_fd;
                            epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &epoll_event_helper);
                        }
                    }
                    
                    // Peer Error
                    if(triggered_event & EPOLLERR)
                    {
                        // close(triggered_fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, triggered_fd, nullptr);
                        printf("peer error: %s\n", strerror(errno));
                        return Server_Socket_State::ERROR;
                    }
                
                    // Ready for reading
                    if((triggered_fd != listen_fd) && (triggered_event & EPOLLIN))
                    {
                        // It peer shudown
                        if(triggered_event & EPOLLRDHUP)
                        {
                            close(triggered_fd);
                            printf("Trigger EPOLLRDHUP, fd %d shutdown \n", triggered_fd);
                            continue;
                        }
                        readable_fd = triggered_fd;
                        return Server_Socket_State::READABLE;
                    }
                }
            }
            break;
        }   // end of swtich()
    }   // end of for(;;)
}

bool Server_Socket::write_to(const int peer_fd, const std::string& data_string)
{
    int send_result = send(peer_fd, data_string.c_str(), data_string.size(), 0);
    
    if(send_result < 0)
    {
        perror("send");
        return false;
    }
    else if(send_result == 0)
    {
        printf("send return 0\n");
        return false;
    }
    
    return true;
}   

std::string Server_Socket::read_from()
{
    return read_from();
}

std::string Server_Socket::read_from(const int peer_fd)
{
    char local_receive_buffer[MAXIMUM_BUFFER_SIZE] = { 0 };
    ssize_t receive_result = recv(peer_fd, &local_receive_buffer, sizeof(local_receive_buffer), 0);
    
    if(receive_result == -1)
    {
        perror("recv");
        return nullptr;
    }
    else if(receive_result == 0)
    {   
        printf("recv return 0\n");
        return nullptr;
    }

    return std::string(local_receive_buffer);
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

std::string Server_Socket::generate_string_from_byte_stream(const std::vector<uint8_t>& byte_stream)
{
    std::string result_string;
    for(const auto& byte : byte_stream)
        result_string += static_cast<char>(byte);
    
    return result_string;
}

bool Server_Socket::set_socket_non_blocking(const int socket_fd)
{
    if(socket_fd < 0)
        return false;

    int file_status_flags = fcntl(socket_fd, F_GETFL, 0);
    if(file_status_flags < 0)
        return false;
    
    file_status_flags |= O_NONBLOCK;
    return (fcntl(socket_fd, F_SETFL, file_status_flags) == 0);
}

int Server_Socket::get_readable_fd() const
{
    return readable_fd;
}