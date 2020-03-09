/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utility.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/08 15:00:35 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/09 19:46:31 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

# include <sys/time.h>
# include <time.h>
# include <sys/socket.h>
# include <stdlib.h>
# include <unistd.h>
# include <sys/types.h>
# include <arpa/inet.h>
# include <fcntl.h>
# include <vector>
# include <string>

typedef struct s_range {
    int min;
    int max;

	s_range(int min=0, int max=0): min(min), max(max) {}
	s_range(const s_range &other): min(other.min), max(other.max) {}
	void set(int min, int max) {
		this->min = min;
		this->max = max;
	}
} Range;

typedef struct s_stats {
	unsigned long sendStart;
	unsigned long recvStart;
	unsigned long lastRecv;
	unsigned long lastSend;
	int totalRead;
	int totalSend;
	int totalRecv;

	s_stats(void): totalRead(0), totalRecv(0), totalSend(0) {}
} Stats;

typedef struct s_output {
	static int idx;
	int fd;
	std::string name;

	s_output(void): fd(-1) {}
	virtual ~s_output() {
		if (fd) {
			close(fd);
			unlink(this->name.c_str());
		}
	}
	bool create(void) {
		this->name = "kuraianto_out" + std::to_string(++s_output::idx);
		if ((this->fd = open(this->name.c_str(), O_TRUNC | O_RDWR | O_APPEND | O_CREAT, 0666)) < 0)
			return (false);
		fcntl(this->fd, F_SETFL, O_NONBLOCK);
		return (true);
	}
} Output;

typedef struct s_socket {
	int fd;
	socklen_t len;
	struct sockaddr_in addr;
	bool ready;

	s_socket(void): fd(-1), ready(false) {}
	bool connect(std::string const &ip, int port) {
		this->len = sizeof(struct sockaddr);
		if ((this->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("kuraianto: socket");
			return (false);
		}
		this->addr.sin_family = AF_INET;
		this->addr.sin_addr.s_addr = inet_addr(ip.c_str());
		this->addr.sin_port = htons(port);
		if (::connect(this->fd, (struct sockaddr*)&this->addr, this->len) < 0) {
			perror("kuraianto: connect");
			return (false);
		}
		fcntl(this->fd, F_SETFL, O_NONBLOCK);
		return (true);
	}
	void disconnect(void) {
		close(fd);
	}
} Socket;

enum FDType {
	FD_READ = 0,
	FD_WRITE = 1
};

typedef struct s_set {
	fd_set readfds;
	fd_set writefds;
	int max;

	s_set(void): max(0) {}
	void zero(void) {
		FD_ZERO(&this->readfds);
		FD_ZERO(&this->writefds);
		this->max = 0;
	}
	void add(FDType type, int fd) {
		FD_SET(fd, (type == FD_READ) ? &this->readfds : &this->writefds);
		if (fd > this->max)
			this->max = fd;
	}
	bool ready(FDType type, int fd) const {
		return (FD_ISSET(fd, (type == FD_READ) ? &this->readfds : &this->writefds));
	}
} SelectSet;

unsigned long getCurrentTime(void);
