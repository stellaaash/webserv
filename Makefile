NAME		=	webserv
CXX			=	c++
CXXFLAGS	=	-Wall -Wextra -Wpedantic -Weffc++ -Wconversion -Wsign-conversion -Werror -std=c++98 -g
RM			=	rm
RMFLAGS		=	-f

SRCDIR		=	src
CFILES		=	$(SRCDIR)/main.cpp \
				$(SRCDIR)/ConnectionManager/Connection.cpp \
				$(SRCDIR)/ConnectionManager/Request.cpp \
				$(SRCDIR)/ConnectionManager/Response.cpp

OBJS		=	$(CFILES:.cpp=.o)

INCLDIR		=	include
IFILES		=

VALGRIND	=	valgrind
VALFLAGS	=	--leak-check=full --show-leak-kinds=all
LOG			=	valgrind.log


all:			$(NAME)

%.o:			%.cpp
				@printf "\rCompiling $<..."
				@$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME):		$(OBJS)
				@printf "\rCompiling $(NAME)..."
				@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
				@printf "\r\n\033[32m$(NAME) compiled.\033[0m\n"

clean:
				@printf "\rCleaning object files"
				@$(RM) $(RMFLAGS) $(OBJS)
				@printf "\rObject files cleaned.\n"

fclean:			clean
				@printf "\rRemoving $(NAME)..."
				@$(RM) $(RMFLAGS) $(NAME)
				@$(RM) $(RMFLAGS) $(LOG)
				@printf "\r$(NAME) Removed.\n"

re:				fclean all

test:			re
				@printf "\n==============================================\n"
				$(VALGRIND) $(VALFLAGS) ./$(NAME) | tee $(LOG)

.PHONY:			all clean fclean re debug test
