CC			=	clang++-6.0

FWARN		=	-Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable
FDEBUG		=	-g
FOPTI		=	-O3 -Ofast
FLAGS		=	$(FWARN) $(FDEBUG) $(FOPTI)

NAME		=	server

SRCS_DIR	=	srcs
OBJS_DIR  	=	objs
INC_DIR		=	includes

SRCS	=	main.cpp \
			Logger.cpp \
			ServerSocketPool.cpp \
			Parser.cpp \
			ByteBuffer.cpp \
			Utils.cpp

INCLUDE	=	$(addprefix $(INC_DIR)/, \
				Logger.hpp \
				ServerSocketPool.hpp \
				Parser.hpp \
				Utils.hpp \
				ByteBuffer.hpp \
				Containers.hpp \
			)

OBJS		= $(SRCS:.cpp=.o)

all:	$(NAME)

$(NAME): $(addprefix $(OBJS_DIR)/, $(OBJS)) $(INCLUDE)
	$(CC) $(FLAGS) $(addprefix $(OBJS_DIR)/, $(OBJS)) -I $(INC_DIR)
	mv a.out $(NAME)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp
	$(CC) $(FLAGS) -c $< -o $@

clean:
	@rm -f $(addprefix $(OBJS_DIR)/, $(OBJS))

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: clean fclean re
