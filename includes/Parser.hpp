#pragma once

#include "header.h"
#include <string>
#include <iostream>

class Parser {
	private:
		Parser() {}
	public:
		std::string	httpRequestToStr(int sock_fd, bool* read);
		static Parser& getInstance();
};