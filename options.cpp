/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Options.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/07 16:43:37 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/08 22:16:57 by ncolomer         ###   ########.fr       */
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

Options::~Options() {
	for (auto &definition : this->requests) {
		delete definition;
	}
}

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
		std::vector<std::string> const &parts = split(rawRequest, ",");
		if (parts.size() == 1 && this->requests.size() > 0) {
			if ((this->requests.back()->repeat = std::stoi(parts[0])) < 0)
				return (false);
			continue ;
		} else if (parts.size() < 3)
			return (false);
		RequestDefinition *def = new RequestDefinition();
		this->requests.push_back(def);
		def->type = parts[0];
		def->url = parts[1];
		for (size_t i = 2; i < parts.size() - 1; ++i)
			def->headers.push_back(parts[i]);
		def->bodySize = std::stol(parts.back());
		if (def->bodySize < 0)
			return (false);
	}
	return (true);
}

bool Options::match(char const *str, char const *against) {
	return (strcmp(str, against) == 0);
}

bool Options::initalize(std::vector<std::string> &files, size_t argc, char const **argv) {
	try {
		Key state = P_NONE;
		size_t i = 1;
		for ( ; i < argc; ++i) {
			if (!state) {
				state = Options::match(argv[i], "-r")	? P_RECV_SIZE :
						Options::match(argv[i], "-s")	? P_SEND_SIZE :
						Options::match(argv[i], "-t")	? P_TIMEOUT   :
						Options::match(argv[i], "-i")	? P_INTERVAL  :
						Options::match(argv[i], "-g")	? P_GENERATE  :
						Options::match(argv[i], "-h")	? P_HEADERS   :
						Options::match(argv[i], "-c")	? P_CHUNK_SIZE :
						Options::match(argv[i], "--no-output") ? P_NO_OUTPUT : P_NONE;
				if (state == P_NONE) {
					break ;
				} else if (state == P_NO_OUTPUT) {
					this->noOutput = true;
					state = P_NONE;
				}
			} else if (state == P_TIMEOUT) {
				this->timeout = std::stoi(argv[i]);
				if (this->timeout < 1)
					return (false);
				state = P_NONE;
			} else if (state == P_MAX_SIZE) {
				this->maxSize = std::stol(argv[i]);
				state = P_NONE;
			} else if (state == P_GENERATE) {
				if (!this->setRequests(argv[i]))
					return (false);
				state = P_NONE;
			} else if (state == P_HEADERS) {
				this->headers = split(argv[i], "#");
				state = P_NONE;
			} else if (state != P_NONE) {
				Range &which =  (state == P_RECV_SIZE) ? this->recvSize :
								(state == P_SEND_SIZE) ? this->sendSize :
								(state == P_INTERVAL)  ? this->interval : this->chunkSize;
				if (this->setRange(which, state, argv[i]))
					return (false);
				if (which.max > this->biggestBufferSize)
					this->biggestBufferSize = which.max;
				state = P_NONE;
			}
		}
		if (!this->setIp(argv[i++]))
			return (false);
		for ( ; i < argc; ++i) {
			files.push_back(argv[i]);
		}
		return (true);
	} catch(const std::exception& e) {
		return (false);
	}
}
