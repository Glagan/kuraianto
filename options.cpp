/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Options.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/07 16:43:37 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/08 16:22:11 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Options.hpp"

Options::Options() {
	this->ip = "127.0.0.1";
	this->port = 0;
	this->recvSize.min = 5;
	this->recvSize.max = 5;
	this->sendSize.min = 5;
	this->sendSize.max = 5;
	this->interval.min = 0;
	this->interval.max = 0;
	this->biggestBufferSize = 5;
	this->maxSize = 0;
	this->timeout = 5;
	this->noOutput = false;
	this->chunkSize.min = 8;
	this->chunkSize.max = 8;
}

Options::~Options() {}

int Options::rangeRand(Range const &range) const {
	return ((rand() % (range.max - range.min + 1)) + range.min);
}

int Options::getSize(Options::Key key) const {
	Range const &range =
		(key == P_RECV_SIZE) ? this->recvSize :
		(key == P_SEND_SIZE) ? this->sendSize :
		(key == P_INTERVAL)  ? this->interval : this->chunkSize;
	if (range.min == range.max)
		return (range.min);
	return (this->rangeRand(range));
}

static std::vector<std::string> split(std::string const &string, std::string const &sep) {
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

bool Options::setIp(char const *value) {
	if (!value || !value[0])
		return (false);
	std::vector<std::string> const &values = split(value, ":");
	if (values.size() > 2)
		return (false);
	if (values.size() == 2)
		this->ip = values[0];
	if ((this->port = std::stoi(values.back())) < 80 || this->port > 65535)
		return (false);
	return (true);
}

bool Options::setRange(Range &range, Key type, char const *value) {
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

bool Options::setRequests(char const *value) {
	std::vector<std::string> const &requests = split(value, ";");
	for (auto const &rawRequest : requests) {
		std::vector<std::string> const &parts = split(value, ",");
		if (parts.size() < 3)
			return (false);
		this->requests.push_back(RequestDefinition());
		RequestDefinition &req = this->requests.back();
		req.type = parts[0];
		req.url = parts[1];
		for (size_t i = 2; i < parts.size() - 1; ++i)
			req.headers.push_back(parts[i]);
		try {
			req.bodySize = std::stoi(parts.back());
		} catch(const std::exception& e) {
			return (false);
		}
		if (req.bodySize < 0)
			return (false);
	}
	return (true);
}

bool Options::initalize(size_t argc, char const **argv) {
	try {
		if (!this->setIp(argv[1]))
			return (false);
		Key state = P_NONE;
		for (size_t i = 2; state >= 0 && i < argc; ++i) {
			if (!state) {
				state = (strcmp(argv[i], "-r") == 0) ? P_RECV_SIZE :
						(strcmp(argv[i], "-s") == 0) ? P_SEND_SIZE :
						(strcmp(argv[i], "-t") == 0) ? P_TIMEOUT   :
						(strcmp(argv[i], "-i") == 0) ? P_INTERVAL  :
						(strcmp(argv[i], "-g") == 0) ? P_GENERATE  :
						(strcmp(argv[i], "-h") == 0) ? P_HEADERS   :
						(strcmp(argv[i], "-c") == 0) ? P_CHUNK_SIZE :
						(strcmp(argv[i], "--no-output") == 0) ? P_NO_OUTPUT : P_INVALID;
			} else if (state == P_TIMEOUT) {
				this->timeout = std::stoi(argv[i]);
				if (this->timeout < 1)
					return (false);
				state = P_NONE;
			} else if (state == P_MAX_SIZE) {
				this->maxSize = std::stoi(argv[i]);
				state = P_NONE;
			} else if (state == P_GENERATE) {
				if (!this->setRequests(argv[i]))
					return (false);
				state = P_NONE;
			} else if (state == P_HEADERS) {
				this->headers = split(argv[i], "#");
				state = P_NONE;
			} else {
				Range &which =  (state == P_RECV_SIZE) ? this->recvSize :
								(state == P_SEND_SIZE) ? this->sendSize :
								(state == P_INTERVAL)  ? this->interval : this->chunkSize;
				if (this->setRange(which, state, argv[i]))
					return (false);
				if (which.max > this->biggestBufferSize)
					this->biggestBufferSize = which.max;
				state = P_NONE;
			}
			if (state == P_NO_OUTPUT) {
				this->noOutput = true;
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
