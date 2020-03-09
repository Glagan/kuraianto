/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utility.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/08 17:19:10 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/09 19:33:30 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utility.hpp"

int Output::idx = 0;

#include <iostream>

unsigned long getCurrentTime(void) {
    struct timespec spec;
	unsigned long ms;

    clock_gettime(CLOCK_REALTIME, &spec);
    ms = spec.tv_sec * 1000;
    long fromNs = (spec.tv_nsec / 1.0e6);
    if (fromNs < 1000)
        ms += fromNs;
	return (ms);
}