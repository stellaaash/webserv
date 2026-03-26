NAME		=	webserv
CXX			=	c++
CXXFLAGS	=	-Wall \
				-Wextra \
				-Wpedantic \
				-Wconversion \
				-Wsign-conversion \
				-Werror \
				-MMD \
				-MP \
				-std=c++98 \
				-g
RM			=	rm
RMFLAGS		=	-f

SRCDIR		=	src
CFILES		=	$(SRCDIR)/main.cpp \
				$(SRCDIR)/ConfigParser/ConfigLexer.cpp \
				$(SRCDIR)/ConfigParser/ConfigParser.cpp \
				$(SRCDIR)/ConnectionManager/HTTP_Message.cpp \
				$(SRCDIR)/ConnectionManager/Connection.cpp \
				$(SRCDIR)/ConnectionManager/Request.cpp \
				$(SRCDIR)/ConnectionManager/RequestParser.cpp \
				$(SRCDIR)/ConnectionManager/Response.cpp \
				$(SRCDIR)/ConnectionManager/ConnectionManager.cpp \
				$(SRCDIR)/ConnectionManager/ConnHandler.cpp \
				$(SRCDIR)/ConnectionManager/Listener.cpp \
				$(SRCDIR)/ConnectionManager/IHandler.cpp \
				$(SRCDIR)/socket_utils.cpp \
				$(SRCDIR)/config.cpp

OBJS		=	$(CFILES:.cpp=.o)

INCLDIR		=	include
IFILES		=

VALGRIND	=	valgrind
VALFLAGS	=	--leak-check=full --show-leak-kinds=all
LOG			=	valgrind.log


all:			$(NAME)

%.o:			%.cpp
				@printf "\rCompiling $<..."
				@$(CXX) $(CXXFLAGS) -I$(INCLDIR) -c $< -o $@

$(NAME):		$(OBJS)
				@printf "\rCompiling $(NAME)..."
				@$(CXX) $(CXXFLAGS) -I$(INCLDIR) $(OBJS) -o $(NAME)
				@printf "\r\n\033[32m$(NAME) compiled.\033[0m\n"

# Include all Makefile dependencies generated using -MMD and -MP
-include $(OBJS:.o=.d)

clean:
				@printf "\rCleaning object files"
				@$(RM) $(RMFLAGS) $(OBJS)
				@$(RM) $(RMFLAGS) $(OBJS:.o=.d)
				@printf "\rObject files cleaned.\n"

fclean:			clean
				@printf "\rRemoving $(NAME)..."
				@$(RM) $(RMFLAGS) $(NAME)
				@$(RM) $(RMFLAGS) $(LOG)
				@printf "\r$(NAME) Removed.\n"

re:				fclean all

CONFIG		?=	config/1.conf

test:			all
				@printf "\n==============================================\n"
				$(VALGRIND) $(VALFLAGS) ./$(NAME) $(CONFIG) | tee $(LOG)

.PHONY:			all clean fclean re debug test
