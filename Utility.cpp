/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utility.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/03/08 17:19:10 by ncolomer          #+#    #+#             */
/*   Updated: 2020/03/08 20:35:06 by ncolomer         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utility.hpp"

int Output::idx = 0;

unsigned long getCurrentTime(void) {
    struct timespec spec;
	unsigned long ms;

    clock_gettime(CLOCK_REALTIME, &spec);
    ms = spec.tv_sec * 1000;
    ms += (spec.tv_nsec / 1.0e6);
	return (ms);
}