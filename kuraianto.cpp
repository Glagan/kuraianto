/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kuraianto.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/05 12:58:54 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/07 19:07:35 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "kuraianto.hpp"
#include "options.hpp"

int g_running = 1;

void showUsage(void) {
	std::cout << "usage: [ip:]port [options]\n\
			\r\r\rOptions:\n\
			\r\r\trange: value | min-max | -max\n\
			\r\r\trequests: [Type,uri,[Header-Name: value,]*bodySize;]+\n\
			\r\r\theaders: [Name: value#]+\n\
			\r\r\t-r range=5\tSet packet size when receiving.\n\
			\r\r\t-s range=5\tSet packet size when sending.\n\
			\r\r\t-t number=5\tMaximum time to wait (s) if there is no response.\n\
			\r\r\t-i range=0\tSet interval (ms) between sending and receiving.\n\
			\r\r\t-m number=0\tMaximum size (bytes) the client can send.\n\
			\r\r\t--no-output\tDo not display received response\n\
			\r\r\t-g requests\tGenerate one or many requests.\n\
			\r\r\tThe next options are only applied on requests generated with -g\n\
			\r\r\t-h headers\tAdd each listed headers to the generated requests.\n\
			\r\r\t-c range=8\tSet the size of sent chunk if the generated request has Transfer-Encoding: chunked\n";
}

bool createOut(Output &out, int idx) {
	out.name = "kuraianto_out" + idx;
	if ((out.fd = open(out.name.c_str(), O_TRUNC | O_RDWR | O_APPEND | O_CREAT, 0666)) < 0)
		return (false);
	fcntl(out.fd, F_SETFL, O_NONBLOCK);
	return (true);
}

void displayOut(Request &req) {
	char buffer[4096];
	Stats &stats = req.stats;

	if (stats.totalRead > 0) {
		std::cout << KYEL "\n###" KNRM "\n" \
			KCYN "Received" KNRM ": " KMAG << stats.totalRecv << " bytes\n" KNRM \
			KBLU "Sent" KNRM ": " KMAG << stats.totalSend << " bytes" KNRM " / " \
			KBLU "Read" KNRM ": " KMAG << stats.totalRead << " bytes\n" KNRM \
			KYEL "###" KNRM << std::flush;
		if (req.output.fd > 0) {
			lseek(req.output.fd, 0, SEEK_SET);
			int lastRead = 0;
			while ((lastRead = read(req.output.fd, buffer, 4096)) > 0)
				write(STDOUT_FILENO, buffer, lastRead);
			std::cout << KYEL "###" KNRM;
		}
		std::cout << '\n';
		unlink(req.output.name.c_str());
		close(req.output.fd);
	}
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

void startSingleRequest(Socket &server, Options &options) {
	Output out;
	if (!options.noOutput && !createOut(out, 1))
		perror("kuraianto: open");
}

void startGeneratedRequests(Socket &server, Options &options) {
	// Open output files
	if (!options.noOutput) {
		int idx = 0;
		for (auto &req : options.requests) {
			if (!createOut(req.output, ++idx)) {
				idx = -1;
				break ;
			}
		}
		if (idx == -1) {
			for (auto &req : options.requests) {
				if (req.output.fd > 0) {
					close(req.output.fd);
					unlink(req.output.name.c_str());
				}
			}
			perror("kuraianto: open");
		}
	}
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

	// Connect to server
	Socket server;
	if (!openSocket(server, options.ip, options.port))
		return (1);
	signal(SIGINT, stop);
	std::cout << "connected to " << options.ip << ":" << options.port << "\n";

	// Start requests
	if (options.requests.size()) {
		startSingleRequest(server, options);
	} else {
		startGeneratedRequests(server, options);
	}

	// done
	close(server.fd);

	// main
	char buffer[options.biggesBufferSize];
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
				if (!stats.totalRecv || !stats.totalSend)
					std::cout << BRED "It's been " << options.timeout << " seconds, what are you doing ?" BNRM;
				else
					std::cout << "Connection expired after " << options.timeout << " seconds, that's too bad (or is it ?)";
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
			int nextSendSize = options.getSize(P_SEND_SIZE);
			if (options.maxSize > 0 && stats.totalSend + nextSendSize > options.maxSize)
				nextSendSize = (stats.totalSend + nextSendSize - options.maxSize);
			if ((stats.lastRead = read(STDIN_FILENO, buffer, nextSendSize)) < 0) {
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
				if (options.maxSize > 0 && stats.totalSend >= options.maxSize) {
					std::cout << "send enough (" << options.maxSize << " bytes), done" << std::endl;
					break ;
				}
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
			while ((stats.lastRead = read(fileout, buffer, options.biggesBufferSize)) > 0) {
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
