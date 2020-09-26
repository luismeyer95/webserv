CC			=	clang++-6.0

FWARN		=	-Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable
FSTD		=	-std=c++11
FDEBUG		=	-g
FOPTI		=	-O3 -Ofast
FLAGS		=	$(FWARN) $(FSTD) $(FDEBUG) $(FOPTI)

NAME		=	server

SRCS_DIR	=	srcs
OBJS_DIR  	=	objs
INC_DIR		=	includes

SRCS	=	main.cpp \
			Logger.cpp \
			ServerSocketPool.cpp \
			RequestParser.cpp \
			ByteBuffer.cpp \
			Utils.cpp \
			Regex/NFA.cpp \
			Regex/NFAState.cpp \
			Regex/PatternValidation.cpp \
			Regex/Regex.cpp \
			Conf/Config.cpp \
			Conf/ConfBlockDirective.cpp \
			Conf/ConfDirective.cpp \
			URL.cpp \
			ErrorCode.cpp \
			RequestRouter.cpp \
			CGI.cpp

INCLUDE	=	$(addprefix $(INC_DIR)/, \
				Logger.hpp \
				ServerSocketPool.hpp \
				RequestParser.hpp \
				Utils.hpp \
				ByteBuffer.hpp \
				Containers.hpp \
				Regex.hpp \
				Conf.hpp \
				URL.hpp \
				ErrorCode.hpp \
				RequestRouter.hpp \
				CGI.hpp \
			)

OBJS		= $(SRCS:.cpp=.o)

all:	$(NAME)

$(NAME): $(addprefix $(OBJS_DIR)/, $(OBJS)) $(INCLUDE)
	$(CC) $(FLAGS) $(addprefix $(OBJS_DIR)/, $(OBJS)) $(addprefix -I, $(INC_DIR))
	mv a.out $(NAME)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp
	$(CC) $(FLAGS) $(addprefix -I, $(INC_DIR)) -c $< -o $@

clean:
	@rm -f $(addprefix $(OBJS_DIR)/, $(OBJS))

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: clean fclean re
