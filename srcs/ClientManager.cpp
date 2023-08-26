#include "ClientManager.hpp"
#include "Http/Handler/Handler.hpp"
#include "Message/Request.hpp"
#include <sys/event.h>
#include <unistd.h>
#include <vector>
#include <iostream>


ClientManager::ClientManager(){};

ClientManager::~ClientManager(){};

Client& ClientManager::getClient(SOCKET client_fd)
{
    return clients.find(client_fd)->second;
}

ClientManager::SOCKET ClientManager::addNewClient(SOCKET client_fd, Server* server, Event* events)
{
    clients[client_fd] = Client();
    clients[client_fd].setFd(client_fd);
    clients[client_fd].setServer(server);
    clients[client_fd].setEvents(events);
    return client_fd;
}

void ClientManager::disconnectClient(SOCKET client_fd)
{
    close(client_fd);
    clients.erase(client_fd);
}

bool ClientManager::readEventProcess(struct kevent& currEvent)
{
    Client* currClient = reinterpret_cast<Client*>(currEvent.udata);
    if (currClient->readMessage())
    {
        addToDisconnectClient(currEvent.ident);
        return false;
    }
    if (currClient->readEventProcess())
    {
        if (currClient->request.is_static)
            clients[currEvent.ident].events->changeEvents(currClient->getClientFd(), EVFILT_READ, EV_DISABLE, 0, 0, currClient);
        if (currClient->request.file_fd == -1)
            return true;
    }
    return false;
}

bool ClientManager::writeEventProcess(struct kevent& currEvent)
{
    Client* currClient = reinterpret_cast<Client*>(currEvent.udata);
    if (currClient->writeEventProcess())
    {
        addToDisconnectClient(currEvent.ident);
        return false;
    }
    if (currClient->isSendBufferEmpty())
    {
        return true;
    }
    return false;
}

int ClientManager::ReqToCgiWriteProcess(struct kevent& currEvent)
{
    Client* client = reinterpret_cast<Client*>(currEvent.udata);
    Request&    request = client->request;
    std::vector<unsigned char>& buffer = request.body;
    const int   size = buffer.size() - request.writeIndex;

    int writeSize = write(currEvent.ident, &buffer[request.writeIndex], size);
    if (writeSize == -1)
    {
        std::cout << currEvent.ident << std::endl; // 7
        std::cout << "write() error" << std::endl;
        std::cout << "errno : " << errno << std::endl;
        client->events->changeEvents(currEvent.ident, EVFILT_WRITE, EV_DISABLE, 0, 0, client);
        return -1;
    }
    request.writeIndex += writeSize;
    if (request.method == "PUT")
    {
        request.RW_file_size += writeSize;
        if (request.RW_file_size == request.file_size)
        {
            close(currEvent.ident);
            client->request.file_fd = -1;
            return 0;
        }
    }
    else if (request.writeIndex == buffer.size())
    {
        request.writeIndex = 0;
        close(currEvent.ident);
        client->request.pipe_fd[1] = -1;
        buffer.clear();
        return 1;
    }
    return 0;
}

int ClientManager::CgiToResReadProcess(struct kevent& currEvent)
{
    const ssize_t BUFFER_SIZE = 65536;

    Client* currClient = reinterpret_cast<Client*>(currEvent.udata);
    std::vector<unsigned char>& readBuffer = currClient->response.body;

    char buffer[BUFFER_SIZE];


    ssize_t ret = read(currEvent.ident, buffer, BUFFER_SIZE);
    if (ret == -1)
        return -1;
    readBuffer.insert(readBuffer.end(), buffer, &buffer[ret]);
    currClient->request.RW_file_size += ret;
    if (currClient->request.is_static && currClient->request.RW_file_size == currClient->request.file_size) {
        return 1;
    }
    return ret == 0;
}

bool ClientManager::isClient(SOCKET client_fd)
{
    return (clients.find(client_fd) != clients.end());
}

void ClientManager::clearClients(void)
{
    std::set<SOCKET>::iterator it = toDisconnectClients.begin();
    for (; it != toDisconnectClients.end(); it++)
    {
        std::map<SOCKET, Client>::iterator ItClient = clients.find(*it);
        if (ItClient->second.request.pipe_fd[1] != -1)
            close(ItClient->second.request.pipe_fd[1]);
        if (ItClient->second.request.pipe_fd_back[0] != -1)
            close(ItClient->second.request.pipe_fd_back[0]);
        if (ItClient->second.request.file_fd != -1)
            close(ItClient->second.request.file_fd);
        disconnectClient(*it);
    }
    toDisconnectClients.clear();
}

void ClientManager::addToDisconnectClient(SOCKET client_fd)
{
    toDisconnectClients.insert(client_fd);
}