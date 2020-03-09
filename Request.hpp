/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/08 14:55:29 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/09 19:42:31 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

# include <iostream>
# include <iomanip>
# include "terminal.h"
# include "Options.hpp"
# include "Utility.hpp"

class Request {
protected:
	static int count;
	static char *buffer;
	std::string unsendBuffer;

	Options const &options;
	Socket server;
	Output output;
	Stats stats;
	unsigned long lastRecap;
	int id;
	bool initialized;
	bool completed;
	bool closed;

	std::ostream &log(void) const;
	virtual bool isReady(SelectSet const &set) const = 0;
	virtual int getNextPacket(void) = 0;
public:
	Request(Options const &options);
	virtual ~Request();
	static void createBuffer(int size);
	static void deleteBuffer(void);

	bool initialize(void);
	void showRecap(void);
	void displayResult(void) const;
	bool isCompleted(void) const;
	bool isClosed(void) const;
	virtual void addToSet(SelectSet &set) const;

	void receive(SelectSet const &set);
	void send(SelectSet const &set);
};

class FileRequest:
	public Request {
protected:
	int fd;

	bool isReady(SelectSet const &set) const;
	int getNextPacket(void);
public:
	FileRequest(Options const &options, int fd);
	virtual ~FileRequest();

	void addToSet(SelectSet &set) const;
};

class GeneratedRequest:
	public Request {
private:
	enum State {
		EMPTY,
		BODY,
		CHUNK,
		DONE
	};
protected:
	RequestDefinition const &definition;
	State state;
	std::string buffer;
	bool isChunked;
	int remainingBody;

	bool isReady(SelectSet const &set) const;
	bool generate(void);
	int getNextPacket(void);
public:
	GeneratedRequest(Options const &options, RequestDefinition const &definition);
	virtual ~GeneratedRequest();
};
