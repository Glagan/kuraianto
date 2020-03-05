# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ncolomer <ncolomer@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2020/03/05 12:58:59 by ncolomer          #+#    #+#              #
#    Updated: 2020/03/05 13:02:47 by ncolomer         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

SRCS			= kuraianto.cpp
OBJS			= $(SRCS:.cpp=.o)
CXX				= clang++
RM				= rm -f
CXXFLAGS		= -std=c++11 -I.
NAME			= kuraianto

all:			$(NAME)

$(NAME):		$(OBJS)
				$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

clean:
				$(RM) $(OBJS)

fclean:			clean
				$(RM) $(NAME)

re:				fclean $(NAME)
