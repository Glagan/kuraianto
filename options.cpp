/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   options.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/07 16:43:37 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/07 18:44:59 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "options.hpp"

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

int rangeRand(Range const &range) {
	return ((rand() % (range.max - range.min + 1)) + range.min);
}

static bool setIp(Options &options, char const *value) {
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

static bool setRange(Range &range, ParamKey type, char const *value) {
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

static bool setRequests(Options &options, char const *value) {
	std::vector<std::string> const &requests = split(value, ";");
	for (auto const &rawRequest : requests) {
		std::vector<std::string> const &parts = split(value, ",");
		if (parts.size() < 3)
			return (false);
		options.requests.push_back(Request());
		Request &req = options.requests.back();
		req.type = parts[0];
		req.url = parts[1];
		req.output = -1;
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

static void setDefault(Options &options) {
	options.ip = "127.0.0.1";
	options.port = 0;
	options.recvSize.min = 5;
	options.recvSize.max = 5;
	options.sendSize.min = 5;
	options.sendSize.max = 5;
	options.interval.min = 0;
	options.interval.max = 0;
	options.biggesBufferSize = 5;
	options.maxSize = 0;
	options.timeout = 5;
	options.noOutput = false;
	options.chunkSize.min = 8;
	options.chunkSize.max = 8;
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
						(strcmp(argv[i], "-g") == 0) ? P_GENERATE  :
						(strcmp(argv[i], "-h") == 0) ? P_HEADERS   :
						(strcmp(argv[i], "-c") == 0) ? P_CHUNK_SIZE :
						(strcmp(argv[i], "--no-output") == 0) ? P_NO_OUTPUT : P_INVALID;
			} else if (state == P_TIMEOUT) {
				options.timeout = std::stoi(argv[i]);
				if (options.timeout < 1)
					return (false);
				state = P_NONE;
			} else if (state == P_MAX_SIZE) {
				options.maxSize = std::stoi(argv[i]);
				state = P_NONE;
			} else if (state == P_GENERATE) {
				if (!setRequests(options, argv[i]))
					return (false);
				state = P_NONE;
			} else if (state == P_HEADERS) {
				options.headers = split(argv[i], "#");
				state = P_NONE;
			} else {
				Range &which =  (state == P_RECV_SIZE) ? options.recvSize :
								(state == P_SEND_SIZE) ? options.sendSize :
								(state == P_INTERVAL)  ? options.interval : options.chunkSize;
				if (setRange(which, state, argv[i]))
					return (false);
				if (which.max > options.biggesBufferSize)
					options.biggesBufferSize = which.max;
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
