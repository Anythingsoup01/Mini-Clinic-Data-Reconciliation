#include "webserver.h"

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

std::string ReadFullRequest(int fd) {
  std::string full_request;
  char buffer[4096];
  ssize_t bytes_received;

  // 1. Read the initial chunk (headers)
  bytes_received = read(fd, buffer, sizeof(buffer));
  if (bytes_received <= 0) return "";

  full_request.append(buffer, bytes_received);

  // 2. Find the header/body separator (\r\n\r\n)
  size_t header_end = full_request.find("\r\n\r\n");

  // 3. Extract Content-Length to know how much body is left
  size_t content_length = 0;
  size_t cl_pos = full_request.find("Content-Length: ");
  if (cl_pos != std::string::npos) {
    content_length = std::stoi(full_request.substr(cl_pos + 16));
  }

  // 4. Calculate how many body bytes we already have
  size_t body_already_read = (header_end != std::string::npos) 
    ? (full_request.length() - (header_end + 4)) : 0;

  // 5. Loop until we have read the full body
  while (body_already_read < content_length) {
    bytes_received = read(fd, buffer, sizeof(buffer));
    if (bytes_received <= 0) break;

    full_request.append(buffer, bytes_received);
    body_already_read += bytes_received;
  }

  return full_request;
}




Webserver::Webserver(uint16_t port) {
  m_FD = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in address;
  address.sin_family=AF_INET;
  address.sin_port=htons(port);
  address.sin_addr.s_addr = INADDR_ANY;

  int opt = 1;
  setsockopt(m_FD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  
  bind(m_FD, (struct sockaddr *)&address, sizeof(address));
  listen(m_FD, 3);
}

Webserver::~Webserver() {
  close(m_FD);
}

void Webserver::HandleRoute(const _Method &method, const std::string &route, const std::function<std::string(const std::string &)>& func) {
  switch (method) {
    case _Method::GET: {
      if (m_GETHandles.contains(route)) return;
      m_GETHandles[route] = func;
      break;
    }
    case _Method::POST: {
      if (m_POSTHandles.contains(route)) return;
      m_POSTHandles[route] = func;
      break;
    }
  }
}

ResponseData Webserver::GetData(int fd) {
  ResponseData data;
  std::string request = ReadFullRequest(fd);

  if (request.length() <= 0) return {};

  size_t spacePos = request.find_first_of(" ");

  std::string methodStr = request.substr(0, spacePos);
  data.Method = methodStr == "GET" ? _Method::GET : _Method::POST;

  data.Route = request.substr(spacePos + 1, request.find_first_of(" ", spacePos + 1) - spacePos - 1);

  size_t pos = request.find("\r\n\r\n"); // HTTP header/body separator
  if (pos != std::string::npos)
    data.JsonData = request.substr(pos + 4);


  return data;

}

void Webserver::Run() {
  while (true) {
    int client_fd = accept(m_FD, nullptr, nullptr);
    if (client_fd == -1) {
      continue; 
    }

    ResponseData data = GetData(client_fd);

    std::string response = "HTTP/1.1 404 Not Found\r\n\r\nPage not found";

    switch (data.Method) {
      case _Method::GET: {
        if (m_GETHandles.contains(data.Route)) {
          response = "HTTP/1.1 200 OK\r\n\r\n";
          response += m_GETHandles[data.Route](data.JsonData);
        }
        break;
      }
      case _Method::POST: {
        if (m_POSTHandles.contains(data.Route)) {
          response = "HTTP/1.1 200 OK\r\n\r\n";
          response += m_POSTHandles[data.Route](data.JsonData);
        }
        break;
      }
    }

    send(client_fd, response.c_str(), response.size(), 0);

    close(client_fd);
  }
}

