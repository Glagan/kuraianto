/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   kuraianto.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/05 12:58:54 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/09 19:49:05 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "kuraianto.hpp"

int g_running = 1;

void showUsage(void) {
	std::cout << "usage: [options] [ip:]port [...files]\n\
		\r\rOptions:\n\
		\r\trange: value | min-max | -max\n\
		\r\trequests: [Type,uri,[Header-Name: value,]*bodySize;[repeat;]]+\n\
		\r\theaders: [Name: value#]+\n\
		\r\t-r range=5\tSet packet size when receiving.\n\
		\r\t-s range=5\tSet packet size when sending.\n\
		\r\t-t number=5\tMaximum time to wait (s) if there is no response.\n\
		\r\t-i range=0\tSet interval (ms) between sending and receiving.\n\
		\r\t-m number=0\tMaximum size (bytes) the client can send.\n\
		\r\t--no-output\tDo not display received response\n\
		\r\t-g requests\tGenerate one or many requests.\n\
		\r\tThe next options are only applied on requests generated with -g\n\
		\r\t-h headers\tAdd each listed headers to the generated requests.\n\
		\r\t-c range=8\tSet the size of sent chunk if the generated request has Transfer-Encoding: chunked\n";
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
	g_running = 0;
	std::cout << "Closing server after interrupt (" << sig << ")\n";
}

int main(int argc, char const **argv) {
	if (argc < 2) {
		showUsage();
		return (1);
	}

	// Options
	Options options;
	Summary summary;
	std::vector<std::string> files;
	if (!options.initalize(files, argc, argv)) {
		std::cerr << "kuraianto: invalid option\n";
		showUsage();
		return (1);
	}
	Request::createBuffer(options.biggestBufferSize);
	srand(time(0));

	std::vector<Request*> requests;
	// Add file requests
	for (auto const &requestFile : files) {
		int fd = open(requestFile.c_str(), O_RDONLY);
		if (fd < 0) {
			std::cout << KRED "Could not open file `" << requestFile << "`" KNRM << std::endl;
		} else {
			requests.push_back(new FileRequest(options, fd));
		}
	}
	// Add stdin Request or Generated Requests
	if (options.requests.size() == 0 && requests.size() == 0)
		requests.push_back(new FileRequest(options, STDIN_FILENO));
	for (auto const &requestDefinition : options.requests)
		for (int i = 0; i < requestDefinition->repeat; ++i)
			requests.push_back(new GeneratedRequest(options, *requestDefinition));

	// Initialize connections
	for (auto &request : requests)
		if (request->initialize())
			++summary.initialized;

	// Avoid doing anything if there is no requests
	if (summary.initialized > 0) {
		std::cout << "Sending " << summary.initialized
				<< " request" << (summary.initialized > 1 ? "s" : "")
				<< " to " << options.ip << ':' << options.port << std::endl;

		// main
		signal(SIGINT, &stop);
		SelectSet set;
		struct timeval timeout;
		int activity;
		int tic = 1;
		while (g_running) {
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;

			// Add each requests to select
			set.zero();
			for (auto const &request : requests)
				if (!request->isClosed())
					request->addToSet(set);

			// Search ready fds
			activity = select(set.max + 1, &set.readfds, &set.writefds, NULL, &timeout);
			if (activity < 0 && errno != EINTR) {
				perror("kuraianto: select");
				break ;
			} else if (activity < 1) {
				std::cout << ((tic++ % 2 == 0) ? "tac" : "tic") << std::endl;
				if (tic > options.timeout) {
					std::cout << "Connection expired after " << options.timeout << " seconds, that's too bad (or is it ?)" << std::endl;
					break ;
				}
				continue ;
			}

			// Send and receive each requests
			for (auto &request : requests) {
				if (!request->isClosed()) {
					request->receive(set);
					if (!request->isCompleted())
						request->send(set);
					request->showRecap();
				}
			}

			// Sleep if needed
			if (options.interval.min > 0)
				usleep(options.getSize(Options::P_INTERVAL) * 1000);
		}

		// done -- display output and clear everything
		for (auto &request : requests) {
			request->displayResult();
			if (request->hasFailed())
				++summary.failed;
			else if (request->isCompleted())
				++summary.completed;
			request->addToSummary(summary);
			delete request;
		}
	} else {
		std::cout << KYEL << "No valid Requests -- nothing sent !" KNRM << std::endl;
	}

	// Summary
	if (summary.initialized != requests.size()) {
		size_t diff = requests.size() - summary.initialized;
		std::cout << KYEL "Failed to initialize " << diff << " request"
				<< (diff > 0 ? "s" : "") << "\n" KNRM;
	}
	if (summary.failed > 0) {
		std::cout << KYEL "Failed to send " << summary.failed << " request"
				<< (summary.failed > 0 ? "s" : "") << "\n" KNRM;
	}
	if (summary.completed > 0) {
		std::cout << KGRN "Successfuly sent " << summary.completed << " request"
				<< (summary.completed > 0 ? "s" : "") << "\n" KNRM;
	}
	if (summary.stats.totalSend > 0 || summary.stats.totalRecv > 0) {
		std::cout << KBLU "Read " KMAG << summary.stats.totalRead << " bytes" KNRM
				", " KCYN "Received " KMAG << summary.stats.totalRecv << " bytes" KNRM
				" and " KBLU "Sent " KMAG << summary.stats.totalSend << " bytes\n" KNRM;
	}

	Request::deleteBuffer();
	std::cout << "Thank you for your service\n";
    return (0);
}
