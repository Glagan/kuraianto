/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kuraianto.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/05 12:58:54 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/05 14:07:16 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <vector>
#include <utility>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include "string.h"
#include "terminal.h"

enum ParamKey {
	P_INVALID = -1,
	P_NONE = 0,
	P_RECV_SIZE,
	P_SEND_SIZE,
	P_TIMEOUT,
	P_INTERVAL,
	P_NO_OUTPUT
};

typedef struct s_range {
    int min;
    int max;
} Range;

int randBetween(Range const &range) {
	return ((rand() % (range.max - range.min + 1)) + range.min);
}

typedef struct s_options {
	std::string ip;
	int port;
	Range recvSize;
	Range sendSize;
	Range interval;
	int maxSize;
	int timeout;
	bool noOutput;

	int getSize(ParamKey type) {
		Range &which =  (type == P_RECV_SIZE) ? this->recvSize :
						(type == P_SEND_SIZE) ? this->sendSize : this->interval;
		if (which.min == which.max)
			return (which.min);
		return (randBetween(which));
	}
} Options;

typedef struct s_socket {
	int fd;
	socklen_t len;
	struct sockaddr_in addr;
} Socket;

enum FDType {
	FD_READ = 0,
	FD_WRITE = 1
};

typedef struct s_set {
	fd_set readfds;
	fd_set writefds;

	void zero(void) {
		FD_ZERO(&this->readfds);
		FD_ZERO(&this->writefds);
	}

	void add(FDType type, int fd) {
		FD_SET(fd, (type == FD_READ) ? &this->readfds : &this->writefds);
	}

	bool ready(FDType type, int fd) {
		return (FD_ISSET(fd, (type == FD_READ) ? &this->readfds : &this->writefds));
	}
} SelectSet;

typedef struct s_stats {
	struct timeval started;
	int lastRead;
	int lastSend;
	int lastRecv;
	int totalRead;
	int totalSend;
	int totalRecv;
} Stats;

int g_running = 1;

void showUsage(void) {
	std::cout << "usage: [ip:]port [options]\n\
				\r\r\r\rOptions:\n\
				\r\r\r\trange: value | min-max | -max\n\
				\r\r\r\t-r range=5\tSet packet size when receiving.\n\
				\r\r\r\t-s range=5\tSet packet size when sending.\n\
				\r\r\r\t-t number=30\tMaximum time to wait (s) if there is no response.\n\
				\r\r\r\t-i range=0\tSet interval (ms) between sending and receiving.\n\
				\r\r\r\t--no-output\tDo not display received response\n";
}

std::vector<std::string> split(std::string const &string, std::string const &sep) {
	size_t pos, offset = 0;
	std::vector<std::string> ret;
	if (string.length() == 0 || sep.length() == 0) {
		ret.push_back(string);
		return (ret);
	}
	while ((pos = string.find(sep, offset)) != std::string::npos) {
		if (offset == pos)
			++offset;
		else {
			ret.push_back(string.substr(offset, pos - offset));
			offset = pos + sep.length();
		}
	}
	if (offset < pos)
		ret.push_back(string.substr(offset, pos - offset));
	return ret;
}

bool setIp(Options &options, char const *value) {
	if (!value || !value[0])
		return (false);
	std::vector<std::string> const &values = split(value, ":");
	if (values.size() > 2)
		return (false);
	if (values.size() == 2)
		options.ip = values[0];
	if ((options.port = std::stoi(values.back())) < 80 || options.port > 65535)
		return (false);
	return (true);
}

bool setRange(Range &range, ParamKey type, char const *value) {
	if (!value || !value[0])
		return (false);
	std::vector<std::string> const &values = split(value, "-");
	if (values.size() > 2)
		return (false);
	if (values.size() == 1) {
		if (value[0] == '-') {
			range.min = 1;
			range.max = std::stoi(values[0]);
		} else {
			range.min = range.max = std::stoi(values[0]);
		}
	} else {
		range.min = std::stoi(values[0]);
		range.max = std::stoi(values[1]);
	}
	if (range.min > range.max
		|| ((range.min < 1 && type != P_INTERVAL)
		|| (range.min >= 0)))
		return (false);
	return (true);
}

void setDefault(Options &options) {
	options.ip = "127.0.0.1";
	options.port = 0;
	options.recvSize.min = 5;
	options.recvSize.max = 5;
	options.sendSize.min = 5;
	options.sendSize.max = 5;
	options.interval.min = 0;
	options.interval.max = 0;
	options.maxSize = 5;
	options.timeout = 30;
	options.noOutput = false;
}

bool setOptions(Options &options, size_t argc, char const **argv) {
	setDefault(options);
	try {
		if (!setIp(options, argv[1]))
			return (false);
		ParamKey state = P_NONE;
		for (size_t i = 2; state >= 0 && i < argc; ++i) {
			if (!state) {
				state = (strcmp(argv[i], "-r") == 0) ? P_RECV_SIZE :
						(strcmp(argv[i], "-s") == 0) ? P_SEND_SIZE :
						(strcmp(argv[i], "-t") == 0) ? P_TIMEOUT   :
						(strcmp(argv[i], "-i") == 0) ? P_INTERVAL  :
						(strcmp(argv[i], "--no-output") == 0) ? P_NO_OUTPUT : P_INVALID;
			} else if (state == P_TIMEOUT) {
				options.timeout = std::stoi(argv[i]);
				if (options.timeout < 1)
					return (false);
				state = P_NONE;
			} else {
				Range &which =  (state == P_RECV_SIZE) ? options.recvSize :
								(state == P_SEND_SIZE) ? options.sendSize : options.interval;
				if (setRange(which, state, argv[i]))
					return (false);
				if (which.max > options.maxSize)
					options.maxSize = which.max;
				state = P_NONE;
			}
			if (state == P_NO_OUTPUT) {
				options.noOutput = true;
				state = P_NONE;
			}
		}
		if (state == P_INVALID)
			return (false);
		return (true);
	} catch(const std::exception& e) {
		return (false);
	}
}

int createOut(void) {
	int fd = open("kuraianto_out", O_TRUNC | O_RDWR | O_APPEND | O_CREAT, 0666);
	if (fd < 0)
		perror("kuraianto: open");
	fcntl(fd, F_SETFL, O_NONBLOCK);
	return (fd);
}

bool openSocket(Socket &socket, std::string const &ip, int port) {
	socket.len = sizeof(struct sockaddr);
	if ((socket.fd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("kuraianto: socket");
		return (false);
	}
	socket.addr.sin_family = AF_INET;
    socket.addr.sin_addr.s_addr = inet_addr(ip.c_str());
    socket.addr.sin_port = htons(port);
	if (connect(socket.fd, (struct sockaddr*)&socket.addr, socket.len) < 0) {
		perror("kuraianto: connect");
		return (false);
	}
	fcntl(socket.fd, F_SETFL, O_NONBLOCK);
	return (true);
}

void setStats(Stats &stats) {
	stats.lastRead = 0;
	stats.lastSend = 0;
	stats.lastRecv = 0;
	stats.totalRead = 0;
	stats.totalSend = 0;
	stats.totalRecv = 0;
}

void outputBytes(int value, int compare, std::string const &prefix) {
	if (value == compare)
		std::cout << "+";
	else if (value)
		std::cout << "  " << compare;
	else
		std::cout << prefix << ' ' << compare << " bytes";
	std::cout << std::flush;
}

void stop(int sig) {
	(void)sig;
	g_running = 0;
	std::cout << "\ntrying to close server after interrupt\n";
}

int main(int argc, char const **argv) {
	if (argc < 2) {
		showUsage();
		return (1);
	}

	// Options
	Options options;
	if (!setOptions(options, argc, argv)) {
		std::cerr << "kuraianto: invalid option\n";
		showUsage();
		return (1);
	}
	srand(time(0));

	// Create output file and open socket
	int fileout = createOut();
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	Socket server;
	if (!openSocket(server, options.ip, options.port))
		return (1);
	signal(SIGINT, stop);
	// std::cout << "Parameters: receive<" << options.recvSize.min << "-" << options.recvSize.max << "> "
	// 		<< "send<" << options.sendSize.min << "-" << options.sendSize.max << "> "
	// 		<< "interval<" << options.interval.min << "-" << options.interval.max << "ms> "
	// 		<< "timeout<" << options.timeout << "s>" << std::endl;
	std::cout << "connected to " << options.ip << ":" << options.port << "\n";

	// Check stdin
	// main
	char buffer[options.maxSize];
	SelectSet set;
	Stats stats;
	setStats(stats);
	gettimeofday(&stats.started, NULL);
	struct timeval timeout;
	int activity;
	int tic = 1;
	std::pair<int, int> already = { 0, 0 };
	bool doneReading = false;
	while (g_running) {
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		set.zero();
		if (!doneReading) {
			set.add(FD_READ, STDIN_FILENO);
			set.add(FD_WRITE, server.fd);
		}
		set.add(FD_READ, server.fd);

		activity = select(server.fd + 1, &set.readfds, &set.writefds, NULL, &timeout);
		if (activity < 0 && errno != EINTR) {
			perror("kuraianto: select");
			close(server.fd);
			return (1);
		} else if (activity < 1) {
			if (already.first || already.second) {
				already.first = already.second = 0;
				std::cout << '\n';
			}
			std::cout << ((tic++ % 2 == 0) ? "tac" : "tic") << std::endl;
			if (tic > options.timeout) {
				std::cout << BRED "It's been " << options.timeout << " seconds, what are you doing ?" BNRM;
				break ;
			}
			continue ;
		}

		// Receive
		if (set.ready(FD_READ, server.fd)) {
			if (already.second)
				std::cout << std::endl;
			if (!already.first)
				std::cout << "reading server response... ";
			if ((stats.lastRecv = read(server.fd, buffer, options.getSize(P_RECV_SIZE))) < 0) {
				std::cout << "kuraianto: error while reading server";
				break ;
			}
			if (stats.lastRecv == 0) {
				std::cout << "\nServer closed the connection, that's fucked up...";
				break ;
			} else if (stats.lastRecv > 0) {
				stats.totalRecv += stats.lastRecv;
				outputBytes(already.first, stats.lastRecv, "received");
				if (!options.noOutput)
					write(fileout, buffer, stats.lastRecv);
			}
			already.first = stats.lastRecv;
			already.second = 0;
		}
		// Send
		if (!doneReading && set.ready(FD_WRITE, server.fd) && set.ready(FD_READ, STDIN_FILENO)) {
			if (already.first)
				std::cout << std::endl;
			if (!already.second)
				std::cout << "reading stdin... ";
			if ((stats.lastRead = read(STDIN_FILENO, buffer, options.getSize(P_SEND_SIZE))) < 0) {
				std::cout << "kuraianto: error while reading stdin";
				break ;
			}
			if (stats.lastRead == 0) {
				if (already.second)
					std::cout << ' ';
				std::cout << "done reading stdin" << std::flush;
				doneReading = true;
			} else {
				stats.totalRead += stats.lastRead;
				if (!already.second)
					std::cout << "sending part of request... ";
				if ((stats.lastSend = write(server.fd, buffer, stats.lastRead)) < 0) {
					std::cout << "kuraianto: error while sending request";
					break ;
				}
				stats.totalSend += stats.lastSend;
				outputBytes(already.second, stats.lastSend, "sent");
			}
			already.second = stats.lastSend;
			already.first = 0;
		}
		if (options.interval.min) {
			std::cout << "...waiting " << options.getSize(P_INTERVAL) << "ms..." << std::flush;
			usleep(options.getSize(P_INTERVAL) * 1000);
		}
	}
	close(server.fd);

	// Display complete output
	if (stats.totalRead > 0) {
		std::cout << KYEL "\n###" KNRM "\n" \
			KCYN "Received" KNRM ": " KMAG << stats.totalRecv << " bytes\n" KNRM \
			KBLU "Sent" KNRM ": " KMAG << stats.totalSend << " bytes" KNRM " / " \
			KBLU "Read" KNRM ": " KMAG << stats.totalRead << " bytes\n" KNRM \
			KYEL "###" KNRM << std::flush;
		if (!options.noOutput) {
			lseek(fileout, 0, SEEK_SET);
			while ((stats.lastRead = read(fileout, buffer, options.maxSize)) > 0) {
				write(STDOUT_FILENO, buffer, stats.lastRead);
			}
			std::cout << KYEL "###" KNRM;
		}
		std::cout << '\n';
	}
	unlink("kuraianto_out");

	close(fileout);
	std::cout << "\nThank you for your service\n";

    return (0);
}
