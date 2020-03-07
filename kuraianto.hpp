/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kuraianto.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/07 16:44:07 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/07 19:06:46 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef KURAIANTO_HPP
# define KURAIANTO_HPP

# include <iostream>
# include <vector>
# include <utility>
# include <unistd.h>
# include <sys/types.h>
# include <stdlib.h>
# include <sys/socket.h>
# include <sys/select.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <fcntl.h>
# include <sys/time.h>
# include <errno.h>
# include <time.h>
# include "string.h"
# include "terminal.h"

typedef struct s_range {
    int min;
    int max;
} Range;

typedef struct s_stats {
	struct timeval started;
	int totalRead;
	int totalSend;
	int totalRecv;
} Stats;

typedef struct s_output {
	int fd;
	std::string name;
} Output;

typedef struct s_request {
	std::string type;
	std::string url;
	std::vector<std::string> headers;
	int bodySize;

	Output output;
	Stats stats;
} Request;

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

#endif