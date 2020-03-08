/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/08 14:55:29 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/08 19:03:17 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

# include <iostream>
# include "terminal.h"
# include "Options.hpp"
# include "Utility.hpp"

class Request {
protected:
	static int count;
	static char *buffer;
	Options const &options;
	Socket server;
	Output output;
	Stats stats;
	int id;
	bool completed;
	bool closed;

	std::ostream &log(std::ostream &out=std::cout) const;
	virtual bool isReady(SelectSet const &set) const = 0;
	virtual int getNextPacket(void) = 0;
public:
	Request(Options const &options);
	virtual ~Request();
	static void createBuffer(int size);
	static void deleteBuffer(void);

	bool initialize(void);
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
protected:
	RequestDefinition const &definition;
	std::string buffer;

	bool isReady(SelectSet const &set) const;
	int getNextPacket(void);
public:
	GeneratedRequest(Options const &options, RequestDefinition const &definition);
	virtual ~GeneratedRequest();
};
