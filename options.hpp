/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Options.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/07 16:45:28 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/08 21:59:53 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

# include "Utility.hpp"

typedef struct s_request_definition {
	std::string type;
	std::string url;
	std::vector<std::string> headers;
	long bodySize;
	int repeat;

	s_request_definition(void): repeat(1) {}
} RequestDefinition;

class Options {
public:
	enum Key {
		P_INVALID = -1,
		P_NONE = 0,
		P_RECV_SIZE,
		P_SEND_SIZE,
		P_TIMEOUT,
		P_INTERVAL,
		P_MAX_SIZE,
		P_GENERATE,
		P_HEADERS,
		P_CHUNK_SIZE,
		P_NO_OUTPUT
	};
private:
	int rangeRand(Range const &range) const;

	bool setIp(char const *value);
	bool setRange(Range &range, Key type, char const *value);
	bool setRequests(char const *value);
public:
	std::string ip;
	int port;
	Range recvSize;
	Range sendSize;
	Range interval;
	int biggestBufferSize;
	long maxSize;
	int timeout;
	std::vector<RequestDefinition*> requests;
	std::vector<std::string> headers;
	Range chunkSize;
	bool noOutput;

	Options();
	virtual ~Options();
	bool initalize(size_t argc, char const **argv);

	int getSize(Key range) const;
};
