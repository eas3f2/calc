#include <vector>
#include <string>
#include <map>
#include <cstdarg>
#define _USE_MATH_DEFINES
#include <math.h>

#include "calc.hpp"

static calc_token calc_iprint(const std::string& name, calc_env& env, calc_func& fe, const std::vector<calc_token>& args) {
	if (args.size()) {
		if (!args[0].value)
			env.printf("%s\n", args[0].string.c_str());
		else
			env.printf("%f\n", args[0].value);
	}
	return 0.0;
}

static calc_token calc_cos(const std::string& name, calc_env& env, calc_func& fe, const std::vector<calc_token>& args) {
	if (args.size()) {
		return cos(args[0].value);
	}
	return 0.0;
}

static calc_token calc_sin(const std::string& name, calc_env& env, calc_func& fe, const std::vector<calc_token>& args) {
	if (args.size()) {
		return sin(args[0].value);
	}
	return 0.0;
}

static calc_token calc_tan(const std::string& name, calc_env& env, calc_func& fe, const std::vector<calc_token>& args) {
	if (args.size()) {
		return tan(args[0].value);
	}
	return 0.0;
}

static calc_token calc_abs(const std::string& name, calc_env& env, calc_func& fe, const std::vector<calc_token>& args) {
	if (args.size()) {
		return abs(args[0].value);
	}
	return 0.0;
}

void calc_env::loadsl() {
	setvar("pi", M_PI, true);
	setfunc("print", calc_iprint);
	setfunc("cos", calc_cos);
	setfunc("sin", calc_sin);
	setfunc("tan", calc_tan);
	setfunc("abs", calc_abs);
}