#pragma once
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <fstream>

class Socket
{
	std::string	s_support_string;
	WSADATA		s_windows_socket_data{};
	WORD		s_windows_socket_version{};
	SOCKET		s_main_listener{};
	sockaddr_in	s_hint{};
	fd_set		s_master_file_descriptor{};
	bool		s_windows_socket_active{};
	WSAEVENT	s_new_event;
public:
	Socket();
	~Socket();

	void s_Socket_Init()
	{
		s_windows_socket_version = MAKEWORD(2, 2);
		WSAStartup(s_windows_socket_version, &s_windows_socket_data);

		s_main_listener = socket(AF_INET, SOCK_STREAM, 0);
		s_hint.sin_family = AF_INET;
		s_hint.sin_port = htons(54000);
		s_hint.sin_addr.S_un.S_addr = INADDR_ANY;

		bind(s_main_listener, reinterpret_cast<sockaddr*>(&s_hint), sizeof(s_hint));
		s_new_event = WSACreateEvent();
		WSAEventSelect(s_main_listener, s_new_event, FD_ACCEPT | FD_CLOSE);
		listen(s_main_listener, SOMAXCONN);
		FD_ZERO(&s_master_file_descriptor);
		FD_SET(s_main_listener, &s_master_file_descriptor);
	}

	void s_Socket_Store_Massage(char buffer[4096], SOCKET socket)
	{
		s_support_string = static_cast<LPCTSTR>(buffer);

		std::ostringstream string_stream;
		string_stream << "#Socket:" << socket << '\n' << "#Message:" << s_support_string << '\n';

		s_support_string = string_stream.str();
		std::fstream storage_target("test.txt", std::ios::app);
		storage_target << s_support_string << '\n';
		storage_target.close();
	}
	void s_Socket_Broadcast(char buffer[4096], SOCKET socket)
	{
		for (unsigned int i = 0; i < s_master_file_descriptor.fd_count; i++)
		{
			SOCKET socket_buffer = s_master_file_descriptor.fd_array[i];

			std::ostringstream string_stream;
			string_stream << "#Socket:" << socket << " #Message:" << buffer << "\r\n";

			s_support_string = string_stream.str();
			send(socket_buffer, s_support_string.c_str(), s_support_string.size() + 1, 0);
		}
	}
	void s_Activate_Socket()
	{
		s_windows_socket_active = true;
		while (s_windows_socket_active)
		{
			fd_set copy_of_master_file_descriptor = s_master_file_descriptor;
			int socket_count = select(0, &copy_of_master_file_descriptor, nullptr, nullptr, nullptr);
			for (int i = 0; i < socket_count; i++)
			{
				SOCKET socket = copy_of_master_file_descriptor.fd_array[i];
				if (socket == s_main_listener)
				{
					SOCKET client = accept(s_main_listener, nullptr, nullptr);
					FD_SET(client, &s_master_file_descriptor);
					std::string welcome_message = "Welcome to the Awesome Server\n";
					send(client, welcome_message.c_str(), welcome_message.size() + 1, 0);
				}
				else
				{
					char buffer[4096];
					ZeroMemory(buffer, 4096);
					int bytes_in = recv(socket, buffer, 4096, 0);
					if (bytes_in <= 0)
					{
						closesocket(socket);
						FD_CLR(socket, &s_master_file_descriptor);
					}
					else
					{
						s_Socket_Store_Massage(buffer, socket);
						s_Socket_Broadcast(buffer, socket);
					}
				}
			}
		}
	}
};


inline Socket::Socket()
{
	s_Socket_Init();
	s_Activate_Socket();
}


inline Socket::~Socket()
{
	FD_CLR(s_main_listener, &s_master_file_descriptor);
	closesocket(s_main_listener);
	for (unsigned int i = 0; s_master_file_descriptor.fd_count > 0; i++)
	{
		std::string msg = "Goodbye";
		SOCKET socket = s_master_file_descriptor.fd_array[i];
		send(socket, msg.c_str(), msg.size() + 1, 0);
		FD_CLR(socket, &s_master_file_descriptor);
		closesocket(socket);
	}
	WSACleanup();
}