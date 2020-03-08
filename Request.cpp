/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/08 14:55:30 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/08 22:41:11 by ncolomer         ###   ########.fr       */
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

std::ostream &Request::log(void) const {
	return (std::cout << KGRN << std::setw(3) << std::left << this->id << "# " KNRM);
}

bool Request::initialize(void) {
	if (this->server.connect(options.ip, options.port))
		this->log() << "Connected to " << options.ip << ":" << options.port << std::endl;
	return (options.noOutput || (!options.noOutput && this->output.create()));
}

void Request::showRecap(void) {
	unsigned long now = getCurrentTime();
	if (now - this->lastRecap > 2000) {
		this->lastRecap = now;
		this->log() << "Sent " KMAG << stats.totalSend << " bytes" KNRM
				" and Received " KMAG << stats.totalRecv << " bytes" KNRM
				" in " KBLU << ((now - stats.sendStart) / 1000.)  << "s" KNRM << std::endl;
	}
}

void Request::displayResult(void) const {
	if (stats.totalSend > 0 || stats.totalRecv > 0) {
		this->log() << KCYN "Received" KNRM ": " KMAG << stats.totalRecv << " bytes" KNRM " in "
				KBLU << ((stats.lastRecv - stats.recvStart) / 1000.) << "s" KNRM " | "
				<< KBLU "Sent" KNRM ": " KMAG << stats.totalSend << " bytes" KNRM;
		if (stats.totalRead)
			std::cout << " | " KBLU "Read" KNRM ": " KMAG << stats.totalRead << " bytes" KNRM;
		std::cout << " | " KGRN "Duration" KNRM ": " KBLU << ((std::max(stats.lastRecv, stats.lastSend) - stats.sendStart) / 1000.) << "s" KNRM << std::endl;
		// Display response if the options is enabled
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
				if (lastRead < 4095 && nlPos > offset)
					std::cout << std::endl;
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
		if (stats.totalRecv == 0) {
			this->log() << "Receiving response\n";
			unsigned long now = getCurrentTime();
			stats.recvStart = now;
			this->lastRecap = now;
		}
		stats.lastRecv = getCurrentTime();
		if (lastRecv == 0) {
			this->log() << "Server closed the connection, that's fucked up...\n";
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
		int size = 0;
		if (unsendBuffer.length() > 0) {
			size = unsendBuffer.length();
			memcpy(Request::buffer, unsendBuffer.c_str(), size);
			unsendBuffer.clear();
		} else {
			size = this->getNextPacket();
			if (size <= 0) {
				this->completed = true;
				this->log() << "Request sent in " KBLU << ((stats.lastSend - stats.sendStart) / 1000.) << "s" KNRM << std::endl;
				return ;
			}
		}
		if (stats.totalSend == 0) {
			this->log() << "Sending request\n";
			unsigned long now = getCurrentTime();
			stats.sendStart = now;
			this->lastRecap = now;
		}
		if ((lastSend = ::send(server.fd, Request::buffer, size, 0)) < 0) {
			std::cerr << "kuraianto: error while sending request\n";
			this->closed = true;
			this->completed = true;
			return ;
		}
		if (lastSend != size) {
			// this->log() << KYEL "Expected to send " << size << " bytes but sent " << lastSend << " bytes"
			// 			<< ", the server might be too slow, try to reduce the sent packet size" KNRM << std::endl;
			this->unsendBuffer.append(Request::buffer + lastSend, size - lastSend);
		}
		stats.lastSend = getCurrentTime();
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
	Request(options), definition(definition), state(State::EMPTY) {
	this->isChunked = (
		std::find(definition.headers.begin(),
				definition.headers.end(),
				"Transfer-Encoding: chunked")
		!= definition.headers.end());
	this->remainingBody = definition.bodySize;
}

GeneratedRequest::~GeneratedRequest() {}

bool GeneratedRequest::isReady(SelectSet const &set) const {
	(void) set;
	return (true);
}

// * https://stackoverflow.com/a/33447587
static std::string int2hex(int value) {
	static const char* digits = "0123456789ABCDEF";
	size_t hexLen = sizeof(int) << 1;
	std::string hex(hexLen, '0');
	for (size_t i = 0, j = (hexLen - 1) * 4; i < hexLen; ++i, j -= 4)
		hex[i] = digits[(value >> j) & 0x0f];
	return (hex);
}

bool GeneratedRequest::generate(void) {
	// Generate Headers
	if (this->state == State::EMPTY) {
		this->buffer = definition.type + " " + definition.url + " HTTP/1.1\r\n";
		for (const auto &header : definition.headers)
			this->buffer.append(header + "\r\n");
		for (const auto &header : options.headers)
			this->buffer.append(header + "\r\n");
		if (!this->isChunked && definition.bodySize > 0)
			this->buffer.append("Content-Length: " + std::to_string(definition.bodySize) + "\r\n");
		this->buffer.append("\r\n");
		this->state =	(this->isChunked)	  ? State::CHUNK :
						(definition.bodySize) ? State::BODY  : State::DONE;
	// Generate (not) chunked body
	} else if (this->state == State::BODY) {
		int generated = (this->remainingBody > 4096) ? 4096 : this->remainingBody;
		this->buffer.append(generated, (char)('c'));
		this->remainingBody -= generated;
		if (this->remainingBody == 0)
			this->state = State::DONE;
	// Generate chunked body
	} else if (this->state == State::CHUNK) {
		if (this->remainingBody == 0) {
			this->buffer.append("00000000\r\n\r\n");
			this->state = State::DONE;
		} else {
			int chunkSize = options.getSize(Options::P_CHUNK_SIZE);
			int generated = (chunkSize > this->remainingBody) ? this->remainingBody : chunkSize;
			this->buffer.append(int2hex(generated) + "\r\n");
			this->buffer.append(generated, (char)('c'));
			this->buffer.append("\r\n");
			this->remainingBody -= generated;
		}
	}
	return (this->state != State::DONE);
}

int GeneratedRequest::getNextPacket(void) {
	int nextSendSize = options.getSize(Options::P_SEND_SIZE);
	if (options.maxSize > 0 && stats.totalSend + nextSendSize > options.maxSize)
		nextSendSize = (stats.totalSend + nextSendSize - options.maxSize);
	if (nextSendSize > this->buffer.length()) {
		bool canGenerate = this->generate();
		while (nextSendSize > this->buffer.length() && canGenerate)
			canGenerate = this->generate();
		nextSendSize = std::min(nextSendSize, (int)this->buffer.length());
	}
	if (nextSendSize > 0) {
		memcpy(Request::buffer, this->buffer.c_str(), nextSendSize);
		this->buffer.erase(0, nextSendSize);
	}
	return (nextSendSize);
}