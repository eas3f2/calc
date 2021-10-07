#include <string>
#include <vector>
#include <map>
#include <iterator>

#include "calc.hpp"

const char* calc_ferror_strings[] = {
	"math error: missing operand",
	"syntax error: bracket mismatch",
	"math error: divide by zero",
	"syntax error: ambiguous variable",
	"syntax error: illegal assignment",
	"syntax error: missing function argument",
	"syntax error: function syntax",
};

std::vector<calc_token> calc_tokenizeln(calc_env& env, const std::string& input) {
	std::vector<calc_token> tokens;
	std::string temp;
	/* in quote */
	bool iq = false;
	/* loop thru each character */
	for (size_t i = 0; i < input.size(); i++) {
		/* start/stop quote literals */
		if (input[i] == '\"') {
			iq = !iq;
			if (temp.size() && temp[0] != '-')
				tokens.push_back(temp);
			temp.clear();
		}
		else if (iq) {
			temp.push_back(input[i]);
		}
		/* punctuation characters are delimiters */
		else if (ispunct(input[i]) && input[i] != '.') {
			/* multi-line break, should be caught by line-breaker */
			if (input[i] == ';')
				return tokens;
			/* if temp contains characters add it to tokens*/
			if (temp.size())
				tokens.push_back(temp);
			/* handle negatives by checking if temp is empty */
			/* good solution for the negatives question it seems */
			if (input[i] == '-') {
				if (!temp.size() && 
					/* logic for fixing subtraction following brackets */
					(tokens.size() && (tokens.back().string[0] != ')' && tokens.back().string[0] != ']'))) {
					temp.push_back('-');
					continue;
				}
			}
			/* clear temp regardless */
			temp.clear();
			/* push delimiter token (punctuation) */
			tokens.push_back(input.substr(i, 1));
		}
		/* add non-whitespace characters to temp */
		else if (!isspace(input[i]))
			temp.push_back(input[i]);
	}
	/* push last token if any into tokens */
	if (temp.size())
		tokens.push_back(temp);
	return tokens;
}

void calc_elimln(calc_env& env, std::vector<calc_token>& elim);

void calc_op(calc_env& env, std::vector<calc_token>& elim, char op, double (*fn)(double a, double b)) {
	/* loop thru each token */
	for (size_t i = 0; i < elim.size(); i++) {
		/* check for operator character */
		if (elim[i].string.size() == 1 && elim[i].string[0] == op) {
			/* if no preceding token -> missing operand */
			if (i == 0) {
				calc_throwerror(env, calc_ferr_mop, elim);
				return;
			}
			else {
				/* check if more than two tokens to perform operation */
				if (elim.size() > 2) {
					/* if the operation isn't division do the operation */
					/* NOTE: places result into first operand token */
					if (op != '/')
						elim[i - 1] = fn(elim[i - 1].value, elim[i + 1].value);
					/* special case check for division by zero -> throw error */
					else if (op == '/') {
						if (elim[i + 1].value == 0.0) {
							calc_throwerror(env, calc_ferr_dbz, elim);
							return;
						}
						else {
							elim[i - 1] = fn(elim[i - 1].value, elim[i + 1].value);
						}
					}
					/* erase operator and second operand tokens */
					elim.erase(elim.begin() + i, elim.begin() + i + 2);
					/* recursive call */
					calc_elimln(env, elim);
				}
				/* less than three tokens -> not enough tokens to perform operation */
				else {
					calc_throwerror(env, calc_ferr_mop, elim);
				}
				return;
			}
		}
	}
}

/* return the numeric result token value */
static calc_token calc_result(const std::vector<calc_token>& tokens) {
	if (tokens.size()) {
		return tokens[0];
	}
	return 0.0;
}

/* eliminate bracket line character */
static void calc_elimbrlnc(calc_env& env, std::vector<calc_token>& elim, char ch) {
	/* loop thru each token */
	for (size_t i = 0; i < elim.size(); i++) {
		/* if token is punctuation bracket character */
		if (elim[i].string[0] == ch) {
			if (i == 0 || !isalpha(elim[i-1].string[0])) {
				/* create a bracket frame */
				std::vector<calc_token> frame;
				/* depth is a long so that it doesn't underflow */
				long depth = 0;
				size_t j;
				/* loop thru each token from the beginning of the open bracket */
				for (j = i; j < elim.size(); j++) {
					/* filter */
					if (elim[j].string.size() == 1) {
						/* open brackets increment depth */
						if (elim[j].string[0] == '(' || elim[j].string[0] == '[')
							depth++;
						/* close brackets decrement depth */
						else if (elim[j].string[0] == ')' || elim[j].string[0] == ']')
							depth--;
					}
					/* the first bracket increments depth to 1 */
					/* the last decrements depth to 0 */
					if (depth <= 0)
						break;
					/* skip the first bracket token in the frame */
					/* closing bracket token is skipped by depth check above */
					if (j != i)
						frame.push_back(elim[j]);
				}
				
				/* recursively eliminate frame to result */
				calc_elimln(env, frame);
				/* place frame result into close bracket token */
				elim[j] = calc_result(frame);
				/* erase [opening bracket to close bracket)*/
				elim.erase(elim.begin() + i, elim.begin() + j);
				/* recursive eliminate */
				calc_elimln(env, elim);
				return;
			}
		}
	}
}

/* eliminate brackets */
static void calc_elimbrln(calc_env& env, std::vector<calc_token>& elim) {
	calc_elimbrlnc(env, elim, '[');
	calc_elimbrlnc(env, elim, '(');
}

/* eliminate functions */
static void calc_elimfuncs(calc_env& env, std::vector<calc_token>& tokens) {
	/* loop thru all tokens */
	for (size_t i = 0; i < tokens.size(); i++) {
		/* check for literal */
		if (isalpha(tokens[i].string[0])) {
			/* check if next token exists */
			if (i+1 < tokens.size()) {
				/* function call left parenthesis */
				if (tokens[i+1].string[0] == '(') {
					calc_func& func = env.funcs.find(tokens[i].string)->second;
					std::vector<std::vector<calc_token>> frames;
					std::vector<calc_token> temp;
					size_t j;
					long depth = 1;
					/* loop thru all tokens following `(` */
					for (j = i + 2; j < tokens.size(); j++) {
						/* depth counter */
						if (tokens[j].string[0] == '(') {
							depth++;
						}
						else if (tokens[j].string[0] == ')') {
							depth--;
							if (!depth)
								break;
						}
						/* if token is a comma then add temp to frames and clear temp */
						if (tokens[j].string[0] == ',') {
							frames.push_back(temp);
							temp.clear();
						}
						/* add token to temp */
						else {
							temp.push_back(tokens[j]);
						}
					}
					/* add last temp */
					if (temp.size()) {
						frames.push_back(temp);
					}
					std::vector<calc_token> args;
					for (size_t z = 0; z < frames.size(); z++) {
						calc_elimln(env, frames[z]);
						args.push_back(calc_result(frames[z]));
					}
					if (func.cfunc) {
						tokens[i] = func.cfunc(tokens[i].string, env, func, args);
					}
					else {
						if (args.size() != func.vars.size()) {
							calc_throwerror(env, calc_ferr_missingarg, tokens);
							return;
						}
						env.afunc = &func;
						for (size_t z = 0; z < args.size(); z++) {
							func.vars[z].value = args[z].value;
						}
						std::vector<calc_token> frame = func.tokens;
						tokens[i].value = calc_evalln(env, frame);
						env.afunc = NULL;
					}
					/* erase from [`(` to `)`]. safe because we count parentheses */
					tokens.erase(tokens.begin() + i + 1, tokens.begin() + j + 1);
				}
			}
		}
	}
}

/* eliminate line tokens */
static void calc_elimln(calc_env& env, std::vector<calc_token>& elim) {
	if (elim.size() <= 1)
		return;

	calc_elimbrln(env, elim);
	calc_elimfuncs(env, elim);
	calc_op(env, elim, '^', [] (double a, double b) { return pow(a, b); });
	calc_op(env, elim, '*', [] (double a, double b) { return a * b; });
	calc_op(env, elim, '/', [] (double a, double b) { return a / b; });
	calc_op(env, elim, '+', [] (double a, double b) { return a + b; });
	calc_op(env, elim, '-', [] (double a, double b) { return a - b; });
}

/* count brackets */
static bool calc_countbrackets(const std::string& input, char open, char close) {
	if (std::count(input.cbegin(), input.cend(), open) !=
		std::count(input.cbegin(), input.cend(), close)) {
		return true;
	}
	return false;
}

static size_t calc_findcountof(const std::vector<calc_token>& tokens, const std::string& cmp) {
	size_t c = 0;
	for (size_t i = 0; i < tokens.size(); i++) {
		if (tokens[i].string.compare(cmp) == 0)
			c++;
	}
	return c;
}

static size_t calc_findfirsttoken(const std::vector<calc_token>& tokens, const std::string& token) {
	for (size_t i = 0; i < tokens.size(); i++) {
		if (tokens[i].string.compare(token) == 0)
			return i;
	}
	return 0;
}

static bool calc_assignfunc(calc_env& env, std::vector<calc_token>& tokens) {
	/*
		assigning func inside func??


	if (env.afunc) {
		calc_throwerror();
		return true;
	}
	*/
	/* TODO: do this better procedurally */
	if (tokens.size() > 4 &&
		tokens[1].string.length() == 1 && tokens[1].string[0] == '(' &&
		calc_findcountof(tokens, ")") != 0 &&
		calc_findcountof(tokens, "=") != 0 &&
		calc_findfirsttoken(tokens, "=") == calc_findfirsttoken(tokens, ")") + 1) {

		std::map<std::string, calc_func>::iterator fit = env.funcs.find(tokens[0].string);
		if (fit != env.funcs.end()) {
			calc_func& func = fit->second;
			if (func.stat) {
				calc_throwerror(env, calc_ferr_illegalassign, tokens);
				return true;
			}
			func.cfunc = NULL;
			func.vars.clear();
		}
		env.funcs[tokens[0].string] = {};
		fit = env.funcs.find(tokens[0].string);
		calc_func& func = fit->second;
		if (tokens[2].string[0] == ')') {
			std::vector<calc_token> frame;
			std::copy(tokens.begin() + 2, tokens.end(), std::back_inserter(frame));
			func.tokens = frame;
		}
		else {
			size_t i;
			for (i = 2; i < tokens.size(); i++) {
				if (tokens[i].string[0] == '(') {
					calc_throwerror(env, calc_ferr_funcsyntax, tokens);
					return true;
				}
				if (tokens[i].string[0] == ',') {
					if (tokens[i - 1].string[0] == '(') {
						calc_throwerror(env, calc_ferr_funcsyntax, tokens);
						return true;
					}
					func.vars.push_back(tokens[i - 1].string);
				}
				if (i+1 < tokens.size() && tokens[i+1].string[0] == ')') {
					func.vars.push_back(tokens[i].string);
					i++;
					break;
				}
				if (tokens[i].string[0] == ')')
					break;
			}
			if (i+1 < tokens.size() && tokens[i+1].string[0] == '=') {
				std::vector<calc_token> frame;
				std::copy(tokens.begin() + i + 2, tokens.end(), std::back_inserter(frame));
				func.tokens = frame;
				return true;
			}
			else {
				calc_throwerror(env, calc_ferr_funcsyntax, tokens);
				return true;
			}
		}
	}
	return false;
}

/* variable assignment controller */
static bool calc_assignvar(calc_env& env, std::vector<calc_token>& tokens, double& result) {
	/* check if tokens follow the variable assignment form: <variable name> = ... */
	if (tokens.size() > 2 && 
		tokens[1].string.length() == 1 && tokens[1].string[0] == '=') {
		/* if first token starts with non-alphabetic character then throw error */
		if (!isalpha(tokens[0].string[0])) {
			calc_throwerror(env, calc_ferr_illegalassign, tokens);
			return true;
		}
		/* name is good */
		else {
			/* check if static variable */
			const std::map<std::string, calc_var>::const_iterator cit = env.vars.find(tokens[0].string);
			if (cit != env.vars.cend()) {
				if (cit->second.stat) {
					calc_throwerror(env, calc_ferr_illegalassign, tokens);
					return true;
				}
			}
			/* build frame and evaluate the expression following `=` */
			std::vector<calc_token> frame;
			std::copy(tokens.begin() + 2, tokens.end(), std::back_inserter(frame));
			result = calc_evalln(env, frame);
			/* assign variable */
			env.vars[tokens[0].string].value = result;
			tokens[0].value = result;
			tokens.erase(tokens.begin() + 1, tokens.end());
			return true;
		}
	}
	return false;
}

/* disambiguate variable values for use in expressions */
static bool calc_disambiguatevartokens(const calc_env& env, std::vector<calc_token>& tokens) {
	/* loop thru all tokens */
	for (size_t i = 0; i < tokens.size(); i++) {
		/* check for alphabetic token */
		if (isalpha(tokens[i].string[0]) || (tokens[i].string.size() > 1 && isalpha(tokens[i].string[1]))) {
			/* find token in vars map */
			std::map<std::string, calc_var>::const_iterator cit;
			if (tokens[i].string[0] == '-') {
				cit = env.vars.find(tokens[i].string.substr(1));
			}
			else {
				cit = env.vars.find(tokens[i].string);
			}
			if (cit != env.vars.cend()) {
				/* set token's numeric value to that of var */
				if (tokens[i].string[0] == '-')
					tokens[i].value = -cit->second.value;
				else
					tokens[i].value = cit->second.value;
			}
			else {
				std::map<std::string, calc_func>::const_iterator cit = env.funcs.find(tokens[i].string);
				if (cit != env.funcs.cend())
					continue;
				if (env.afunc) {
					bool found = false;
					for (size_t j = 0; j < env.afunc->vars.size(); j++) {
						if (env.afunc->vars[j].key.compare(tokens[i].string) == 0) {
							tokens[i].value = env.afunc->vars[j].value;
							found = true;
							break;
						}
					}
					if (found)
						continue;
				}
				/* unknown token */
				env.printf("calc_disambiguatevartokens: could not disambiguate `%s`\n", tokens[i].string.c_str());
				return true;
			}
		}
	}
	return false;
}

/* evaluate line tokens */
double calc_evalln(calc_env& env, std::vector<calc_token>& tokens) {
	double result;
	if (calc_assignfunc(env, tokens)) {
		return 0.0;
	}
	if (calc_assignvar(env, tokens, result)) {
		return result;
	}
	if (calc_disambiguatevartokens(env, tokens)) {
		calc_throwerror(env, calc_ferr_ambigvar, tokens);
		return 0.0;
	}
	calc_elimln(env, tokens);
	env.line++;
	return calc_result(tokens).value;
}

/* evaluate line string; multiline disabled */
double calc_evalln(calc_env& env, const std::string& input) {
	env.lerr = 0;
	std::vector<calc_token> tokens;
	if (calc_countbrackets(input, '[', ']') ||
		calc_countbrackets(input, '(', ')')) {
		calc_throwerror(env, calc_ferr_brmm, tokens);
		return 0.0;
	}
	tokens = calc_tokenizeln(env, input);
	if (env.isflagset(calc_flag_dbg_printtokens)) {
		env.printf("line #%d\n", env.line);
		for (size_t i = 0; i < tokens.size(); i++) {
			env.printf("`%s`\n", tokens[i].string.c_str());
		}
	}
	return calc_evalln(env, tokens);
}

/* evaluate lines */
double calc_eval(calc_env& env, const std::string& lines) {
	env.line = 0;
	env.lerr = 0;
	double result = 0.0;
	std::string temp;
	for (size_t i = 0; i < lines.size(); i++) {
		if (lines[i] == ';') {
			result = calc_evalln(env, temp);
			temp.clear();
			continue;
		}
		temp.push_back(lines[i]);
	}
	if (temp.length()) {
		return calc_evalln(env, temp);
	}
	return result;
}


