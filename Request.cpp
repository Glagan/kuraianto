/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/08 14:55:30 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/08 19:26:11 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"

int Request::count = 0;
char *Request::buffer = nullptr;
void Request::createBuffer(int size) {
	if (!Request::buffer)
		Request::buffer = new char[size];
}
void Request::deleteBuffer(void) {
	delete[] Request::buffer;
}

Request::Request(Options const &options):
	options(options), completed(false), closed(false) {
	this->id = ++Request::count;
}

Request::~Request() {
	this->server.disconnect();
}

std::ostream &Request::log(std::ostream &out) const {
	return (out << KGRN << this->id << " # " KNRM);
}

bool Request::initialize(void) {
	if (this->server.connect(options.ip, options.port))
		this->log() << "Connected to " << options.ip << ":" << options.port << std::endl;
	return (!options.noOutput && this->output.create());
}

void Request::displayResult(void) const {
	if (stats.totalSend > 0 || stats.totalRecv > 0) {
		this->log() << KCYN "Received" KNRM ": " KMAG << stats.totalRecv << " bytes" KNRM " | "
				<< KBLU "Sent" KNRM ": " KMAG << stats.totalSend << " bytes" KNRM;
		if (stats.totalRead)
			std::cout << " | " KBLU "Read" KNRM ": " KMAG << stats.totalRead << " bytes" KNRM;
		std::cout << std::endl;
		if (!options.noOutput && stats.totalRecv > 0) {
			static char buffer[4096];
			int lastRead = 0;
			char *pos = NULL;
			int offset = 0;
			int nlPos = 0;
			lseek(output.fd, 0, SEEK_SET);
			// Display received response line by line, with the current Request prefix
			this->log() << KYEL "### RESPONSE" KNRM << std::endl;
			this->log();
			while ((lastRead = read(output.fd, buffer, 4095)) > 0) {
				buffer[lastRead] = 0;
				while ((pos = (char*)memchr(buffer + offset, '\n', lastRead - offset))) {
					nlPos = (pos - buffer);
					if (nlPos > 0)
						std::cout << std::string(buffer + offset, nlPos - offset) << std::endl;
					offset = nlPos + 1;
					if (offset < lastRead)
						this->log();
				}
				if (offset > 0 && offset < lastRead) {
					std::cout << std::string(buffer + offset, lastRead - offset);
				} else if (!pos && !offset)
					std::cout << std::string(buffer, lastRead);
				nlPos = 0;
				offset = 0;
			}
			this->log() << KYEL "### END OF RESPONSE" KNRM << std::endl;
		}
	} else
		this->log() << KYEL "Nothing sent nor received" KNRM << std::endl;
}

bool Request::isCompleted(void) const {
	return (this->completed);
}

bool Request::isClosed(void) const {
	return (this->closed);
}

void Request::addToSet(SelectSet &set) const {
	if (!this->completed)
		set.add(FD_WRITE, server.fd);
	set.add(FD_READ, server.fd);
}

void Request::receive(SelectSet const &set) {
	static int lastRecv = 0;
	if (set.ready(FD_READ, server.fd)) {
		if ((lastRecv = ::recv(server.fd, Request::buffer, options.getSize(Options::P_RECV_SIZE), 0)) < 0) {
			std::cerr << "kuraianto: error while receiving from server\n";
			this->closed = true;
			return ;
		}
		if (lastRecv == 0) {
			this->log() << "Server closed the connection, that's fucked up...";
			this->closed = true;
			return ;
		}
		stats.totalRecv += lastRecv;
		if (!options.noOutput)
			write(output.fd, Request::buffer, lastRecv);
	}
}

void Request::send(SelectSet const &set) {
	static int lastSend = 0;
	if (this->isReady(set) && set.ready(FD_WRITE, server.fd)) {
		int size = this->getNextPacket();
		if (size <= 0) {
			this->completed = true;
			this->log() << "Request completed" << std::endl;
			return ;
		}
		if ((lastSend = ::send(server.fd, Request::buffer, size, 0)) < 0) {
			std::cerr << "kuraianto: error while sending request\n";
			this->closed = true;
			this->completed = true;
			return ;
		}
		stats.totalSend += lastSend;
		if (options.maxSize > 0 && stats.totalSend >= options.maxSize)
			this->completed = true;
	}
}

FileRequest::FileRequest(Options const &options, int fd):
	Request(options), fd(fd) {}

FileRequest::~FileRequest() {
	close(fd);
}

bool FileRequest::isReady(SelectSet const &set) const {
	return (set.ready(FD_READ, this->fd));
}

int FileRequest::getNextPacket(void) {
	static int lastRead = 0;
	int nextSendSize = options.getSize(Options::P_SEND_SIZE);
	if (options.maxSize > 0 && stats.totalSend + nextSendSize > options.maxSize)
		nextSendSize = (stats.totalSend + nextSendSize - options.maxSize);
	if ((lastRead = read(this->fd, Request::buffer, nextSendSize)) < 0) {
		std::cerr << "kuraianto: error while reading file\n";
		return (-1);
	}
	stats.totalRead += lastRead;
	return (lastRead);
}

void FileRequest::addToSet(SelectSet &set) const {
	this->Request::addToSet(set);
	if (!this->completed)
		set.add(FD_READ, this->fd);
}

GeneratedRequest::GeneratedRequest(Options const &options, RequestDefinition const &definition):
	Request(options), definition(definition) {}

GeneratedRequest::~GeneratedRequest() {}

bool GeneratedRequest::isReady(SelectSet const &set) const {
	(void) set;
	return (true);
}

int GeneratedRequest::getNextPacket(void) {
	static int lastMade = 0;
	return (lastMade);
}