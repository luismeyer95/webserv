all:
	@clang++-6.0 -Wall -Wextra -Werror -lpthread cli_tool.cpp -o client

fclean:
	@rm -f client

re: fclean all

.PHONY: fclean re