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
				$(SRCDIR)/config.cpp \
				$(SRCDIR)/Logger.cpp \
				$(SRCDIR)/ConfigParser/config_lexer.cpp \
				$(SRCDIR)/ConfigParser/config_parser.cpp \
				$(SRCDIR)/ConnectionManager/Connection.cpp \
				$(SRCDIR)/ConnectionManager/ConnectionHandler.cpp \
				$(SRCDIR)/ConnectionManager/ConnectionManager.cpp \
				$(SRCDIR)/ConnectionManager/HttpMessage.cpp \
				$(SRCDIR)/ConnectionManager/IHandler.cpp \
				$(SRCDIR)/ConnectionManager/Listener.cpp \
				$(SRCDIR)/ConnectionManager/Request.cpp \
				$(SRCDIR)/ConnectionManager/Response.cpp \
				$(SRCDIR)/FileManager/helpers.cpp \
				$(SRCDIR)/FileManager/listing.cpp \
				$(SRCDIR)/FileManager/read.cpp \
				$(SRCDIR)/FileManager/resolve.cpp \
				$(SRCDIR)/FileManager/write.cpp \
				$(SRCDIR)/RequestParser/request_parser.cpp \
				$(SRCDIR)/RequestParser/helpers.cpp \
				$(SRCDIR)/socket_utils.cpp \

INCLDIR		=	include
BUILDDIR	=	build
IFILES		=

VALGRIND	=	valgrind
VALFLAGS	=	--leak-check=full --show-leak-kinds=all
LOG			=	valgrind.log

OBJS		=	$(CFILES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

all:			$(NAME)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
				@mkdir -p $(dir $@)
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
				@$(RM) $(RMFLAGS) -r $(BUILDDIR)
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
