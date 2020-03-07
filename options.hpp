/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   options.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/07 16:45:28 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/07 16:54:16 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef OPTIONS_HPP
# define OPTIONS_HPP

# include "kuraianto.hpp"

enum ParamKey {
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

int rangeRand(Range const &range);

typedef struct s_options {
	std::string ip;
	int port;
	Range recvSize;
	Range sendSize;
	Range interval;
	int biggesBufferSize;
	int maxSize;
	int timeout;
	std::vector<Request> requests;
	std::vector<std::string> headers;
	Range chunkSize;
	bool noOutput;

	int getSize(ParamKey type) {
		Range &which =  (type == P_RECV_SIZE) ? this->recvSize :
						(type == P_SEND_SIZE) ? this->sendSize :
						(type == P_INTERVAL)  ? this->interval : this->chunkSize;
		if (which.min == which.max)
			return (which.min);
		return (rangeRand(which));
	}
} Options;

bool setOptions(Options &options, size_t argc, char const **argv);

#endif