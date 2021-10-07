#pragma once

class calc_token {
public:
	calc_token() {
		value = 0.0;
	}

	calc_token(double _val) {
		value = _val;
		string = std::to_string(_val);
	}

	calc_token(const std::string& s) {
		value = atof(s.c_str());
		string = s;
	}

	calc_token(const char* s) {
		value = atof(s);
		string = s;
	}

	double value;
	std::string string;
};

typedef void (*calc_print)(const char *_format, ...);

enum {
	calc_ferr_mop = 1, /* mission operand */
	calc_ferr_brmm, /* bracket mismatch */
	calc_ferr_dbz, /* divide by zero */
	calc_ferr_ambigvar, /* ambiguous var */
	calc_ferr_illegalassign, /* assigning illegally */
	calc_ferr_missingarg, /* missing argument */
	calc_ferr_funcsyntax, /* unknown function syntax */
};

const char* calc_ferror_strings[];

typedef calc_token (*calc_cfunc)(
	const std::string& name,
	class calc_env& env,
	class calc_func& fe,
	const std::vector<calc_token>& args
);

class calc_funcvar {
public:
	calc_funcvar(const std::string& _key) {
		key = _key;
		value = 0.0;
	}

	std::string key;
	double value;
};

class calc_func {
public:
	calc_func() {
		cfunc = NULL;
		stat = false;
	}

	calc_func(calc_cfunc _cfunc, bool _static) {
		cfunc = _cfunc;
		stat = _static;
	}

	std::vector<calc_funcvar> vars; /* argument variables precede */
	std::vector<calc_token> tokens; /* expression tokens */
	calc_cfunc cfunc; /* c callback */
	bool stat;
};

class calc_var {
public:
	calc_var() {
		value = 0.0;
		stat = false;
	}

	calc_var(double _val, bool _static) {
		value = _val;
		stat = _static;
	}

	double value;
	bool stat;
};

enum {
	calc_flag_dbg_printtokens = (1 << 0),
};

class calc_env {
public:
	calc_env(calc_print _fp = NULL, uint32_t _flags = 0) {
		line = 0;
		flags = _flags;
		lerr = 0;
		fp = _fp;
		afunc = NULL;
	}

	template<class ... varargs>
	void printf(const char* _format, varargs ... args) const {
		if (fp)
			fp(_format, args ...);
	}

	void setvar(const std::string& key, double val, bool _static) {
		vars[key] = calc_var(val, _static);
	}

	void setfunc(const std::string& key, calc_cfunc cfunc, bool _static = true) {
		funcs[key] = calc_func(cfunc, _static);
	}

	bool isflagset(uint32_t flag) const {
		return (flags & flag) != 0;
	}

	/* load standard library */
	void loadsl();

	uint32_t line; /* line number */
	uint32_t flags; /* flags */
	uint32_t lerr; /* last error */
	calc_print fp; /* print function */
	std::map<std::string, calc_var> vars; /* variables */
	std::map<std::string, calc_func> funcs; /* functions */
	calc_func *afunc; /* active function */
};

#define calc_throwerror(env, err, tokens) { \
 	env.lerr = err; \
	env.printf(__FUNCTION__ ": fatal error " #err " thrown\n\t*** %s\n\t*** line %d\n\t*** haulted\n", calc_ferror_strings[err - 1], env.line); \
	tokens.clear(); \
}

double calc_evalln(calc_env& env, std::vector<calc_token>& tokens);
double calc_evalln(calc_env& env, const std::string& input);
double calc_eval(calc_env& env, const std::string& lines);