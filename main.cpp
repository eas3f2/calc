#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstdarg>
#define _USE_MATH_DEFINES
#include <math.h>

#include "calc.hpp"

static bool inhere = true;

void print(const char* _format, ...) {
	va_list va;
	va_start(va, _format);
	vfprintf_s(stdout, _format, va);
	va_end(va);
}

static calc_token exitfn(const std::string& name,
	class calc_env& env,
	class calc_func& fe,
	const std::vector<calc_token>& args) {
	inhere = false;
	return 0.0;
}

static calc_token clear(const std::string& name,
	class calc_env& env,
	class calc_func& fe,
	const std::vector<calc_token>& args) {
	system("cls");
	return 0.0;
}

int main(int argc, char **argv) {
	calc_env env(print);
	env.loadsl();
	env.setfunc("exit", exitfn);
	env.setfunc("clear", clear);
	std::string input;
	double result;
	while (inhere) {
		std::getline(std::cin, input);
		result = calc_eval(env, input);
		printf("=%f\n", result);
	}
}	