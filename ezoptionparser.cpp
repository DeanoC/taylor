#include "ezoptionparser.hpp"
#include "al2o3_tinystl/algorithm.hpp"
#include "al2o3_vfile/vfile.hpp"
#include "al2o3_vfile/memory.h"

#include <limits> // TODO use replacement
namespace ez {

// Create unique ids with static and still allow single header that avoids multiple definitions linker error.
class OptionParserIDGenerator {
public:
	static OptionParserIDGenerator &instance() {
		static OptionParserIDGenerator Generator;
		return Generator;
	}
	short next() { return ++_id; }
private:
	OptionParserIDGenerator() : _id(-1) {}
	short _id;
};

static inline bool isdigit(const tinystl::string & s, int i=0) {
	int n = s.length();
	for(; i < n; ++i)
		switch(s[i]) {
		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8': case '9': break;
		default: return false;
		}

	return true;
}

static bool isdigit(const tinystl::string * s, int i=0) {
	int n = s->length();
	for(; i < n; ++i)
		switch(s->at(i)) {
		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8': case '9': break;
		default: return false;
		}

	return true;
}

/*
Compare strings for opts, so short opt flags come before long format flags.
For example, -d < --dimension < --dmn, and also lower come before upper. The default STL tinystl::string compare doesn't do that.
*/
static bool CmpOptStringPtr(tinystl::string * s1, tinystl::string * s2) {
	int c1,c2;
	const char *s=s1->c_str();
	for(c1=0; c1 < (long int)s1->size(); ++c1)
		if (isalnum(s[c1])) // locale sensitive.
			break;

	s=s2->c_str();
	for(c2=0; c2 < (long int)s2->size(); ++c2)
		if (isalnum(s[c2]))
			break;

	// Test which has more symbols before its name.
	if (c1 > c2)
		return false;
	else if (c1 < c2)
		return true;

	// Both have same number of symbols, so compare first letter.
	char char1 = s1->at(c1);
	char char2 = s2->at(c2);
	char lo1 = tolower(char1);
	char lo2 = tolower(char2);

	if (lo1 != lo2)
		return lo1 < lo2;

	// Their case doesn't match, so find which is lower.
	char up1 = isupper(char1);
	char up2 = isupper(char2);

	if (up1 && !up2)
		return false;
	else if (!up1 && up2)
		return true;

	return (s1->compare(*s2)<0);
}

/*
Makes a vector of strings from one string,
splitting at (and excluding) delimiter "token".
*/
static void SplitDelim( const tinystl::string& s, const char token, tinystl::vector<tinystl::string*> * result) {
	tinystl::string::const_iterator i = s.begin();
	tinystl::string::const_iterator j = s.begin();
	const tinystl::string::const_iterator e = s.end();

	while(i!=e) {
		while(i!=e && *i++!=token);
		tinystl::string *newstr = new tinystl::string(j, i);
		if (newstr->at(newstr->size()-1) == token) newstr->resize(newstr->size()-1);
		result->push_back(newstr);
		j = i;
	}
}

// Variant that uses deep copies and references instead of pointers (less efficient).
static void SplitDelim( const tinystl::string& s, const char token, tinystl::vector<tinystl::string> & result) {
	tinystl::string::const_iterator i = s.begin();
	tinystl::string::const_iterator j = s.begin();
	const tinystl::string::const_iterator e = s.end();

	while(i!=e) {
		while(i!=e && *i++!=token);
		tinystl::string newstr(j, i);
		if (newstr.at(newstr.size()-1) == token) newstr.resize(newstr.size()-1);
		result.push_back(newstr);
		j = i;
	}
}

static void ToU1(tinystl::string ** strings, uint8_t * out, int n) {
	for(int i=0; i < n; ++i) {
		out[i] = (uint16_t) strtol(strings[i]->c_str(), NULL, 0);
	}
}

static void ToS1(tinystl::string ** strings, int8_t * out, int n) {
	for(int i=0; i < n; ++i) {
		out[i] = (int8_t) strtol(strings[i]->c_str(), NULL, 0);
	}
}

static void ToU2(tinystl::string ** strings, uint16_t * out, int n) {
	for(int i=0; i < n; ++i) {
		out[i] = (uint16_t) strtol(strings[i]->c_str(), NULL, 0);
	}
}

static void ToS2(tinystl::string ** strings, int16_t * out, int n) {
	for(int i=0; i < n; ++i) {
		out[i] = (int16_t) strtol(strings[i]->c_str(), NULL, 0);
	}
}

static void ToS4(tinystl::string ** strings, int32_t * out, int n) {
	for(int i=0; i < n; ++i) {
		out[i] = (int32_t) strtol(strings[i]->c_str(), NULL, 0);
	}
}

static void ToU4(tinystl::string ** strings, uint32_t * out, int n) {
	for(int i=0; i < n; ++i) {
		out[i] = (uint32_t)strtoul(strings[i]->c_str(), NULL, 0);
	}
}

static void ToS8(tinystl::string ** strings, int64_t * out, int n) {
	for(int i=0; i < n; ++i) {
		out[i] = (int64_t)strtoll(strings[i]->c_str(), NULL, 0);
	}
}

static void ToU8(tinystl::string ** strings, uint64_t* out, int n) {
	for(int i=0; i < n; ++i) {
		out[i] = (uint64_t)strtoull(strings[i]->c_str(), NULL, 0);
	}
}

static void ToF(tinystl::string ** strings, float * out, int n) {
	for(int i=0; i < n; ++i) {
		out[i] = (float)atof(strings[i]->c_str());
	}
}

static void ToD(tinystl::string ** strings, double * out, int n) {
	for(int i=0; i < n; ++i) {
		out[i] = (double)atof(strings[i]->c_str());
	}
}

static void StringsToInt64s(tinystl::vector<tinystl::string> & strings, tinystl::vector<int64_t> & out) {
	for(int i=0; i < (long int)strings.size(); ++i) {
		out.push_back(strtoll(strings[i].c_str(),0,0));
	}
}
static void StringsToInt64s(tinystl::vector<tinystl::string*> * strings, tinystl::vector<int64_t> * out) {
	for(int i=0; i < (long int)strings->size(); ++i) {
		out->push_back(strtoll(strings->at(i)->c_str(),0,0));
	}
}
static void StringsToFloats(tinystl::vector<tinystl::string> & strings, tinystl::vector<float> & out) {
	for(int i=0; i < (long int)strings.size(); ++i) {
		out.push_back(atof(strings[i].c_str()));
	}
}
static void StringsToFloats(tinystl::vector<tinystl::string*> * strings, tinystl::vector<float> * out) {
	for(int i=0; i < (long int)strings->size(); ++i) {
		out->push_back(atof(strings->at(i)->c_str()));
	}
}

static void StringsToDoubles(tinystl::vector<tinystl::string> & strings, tinystl::vector<double> & out) {
	for(int i=0; i < (long int)strings.size(); ++i) {
		out.push_back(atof(strings[i].c_str()));
	}
}

static void StringsToDoubles(tinystl::vector<tinystl::string*> * strings, tinystl::vector<double> * out) {
	for(int i=0; i < (long int)strings->size(); ++i) {
		out->push_back(atof(strings->at(i)->c_str()));
	}
}

static void StringsToStrings(tinystl::vector<tinystl::string*> * strings, tinystl::vector<tinystl::string> * out) {
	for(int i=0; i < (long int)strings->size(); ++i) {
		out->push_back( *strings->at(i) );
	}
}

/* ################################################################### */
static void ToLowerASCII(tinystl::string & s) {
	int n = s.size();
	int i=0;
	char c;
	for(; i < n; ++i) {
		c = s[i];
		if(c<='Z' && c>='A')
			s[i] = c+32;
	}
}

/* ################################################################### */
static char** CommandLineToArgvA(char* CmdLine, int* _argc) {
	char** argv;
	char*  _argv;
	unsigned long   len;
	unsigned long   argc;
	char   a;
	unsigned long   i, j;

	bool  in_QM;
	bool  in_TEXT;
	bool  in_SPACE;

	len = strlen(CmdLine);
	i = ((len+2)/2)*sizeof(void*) + sizeof(void*);

	argv = (char**)malloc(i + (len+2)*sizeof(char));

	_argv = (char*)(((unsigned char*)argv)+i);

	argc = 0;
	argv[argc] = _argv;
	in_QM = false;
	in_TEXT = false;
	in_SPACE = true;
	i = 0;
	j = 0;

	while( (a = CmdLine[i]) ) {
		if(in_QM) {
			if( (a == '\"') ||
					(a == '\'')) // rsz. Added single quote.
			{
				in_QM = false;
			} else {
				_argv[j] = a;
				j++;
			}
		} else {
			switch(a) {
			case '\"':
			case '\'': // rsz. Added single quote.
				in_QM = true;
				in_TEXT = true;
				if(in_SPACE) {
					argv[argc] = _argv+j;
					argc++;
				}
				in_SPACE = false;
				break;
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				if(in_TEXT) {
					_argv[j] = '\0';
					j++;
				}
				in_TEXT = false;
				in_SPACE = true;
				break;
			default:
				in_TEXT = true;
				if(in_SPACE) {
					argv[argc] = _argv+j;
					argc++;
				}
				_argv[j] = a;
				j++;
				in_SPACE = false;
				break;
			}
		}
		i++;
	}
	_argv[j] = '\0';
	argv[argc] = NULL;

	(*_argc) = argc;
	return argv;
};

/* ------------------------------------------------------------------- */
OptionValidator::~OptionValidator() {
	reset();
};
/* ------------------------------------------------------------------- */
void OptionValidator::reset() {
#define CLEAR(TYPE,P) case TYPE: if (P) delete [] P; P = 0; break;
	switch(type) {
	CLEAR(S1,s1);
	CLEAR(U1,u1);
	CLEAR(S2,s2);
	CLEAR(U2,u2);
	CLEAR(S4,s4);
	CLEAR(U4,u4);
	CLEAR(S8,s8);
	CLEAR(U8,u8);
	CLEAR(F,f);
	CLEAR(D,d);
	case T:
		for(int i=0; i < size; ++i)
			delete t[i];

		delete [] t;
		t = 0;
		break;
	default: break;
	}

	size = 0;
	op = NOOP;
	type = NOTYPE;
}

/* ------------------------------------------------------------------- */
OptionValidator::OptionValidator(char _type) : s1(0), op(0), quiet(0), type(_type), size(0), insensitive(0) {
	id = OptionParserIDGenerator::instance().next();
}
/* ------------------------------------------------------------------- */
OptionValidator::OptionValidator(char _type, char _op, const uint8_t *list, int _size) : s1(0), op(_op), quiet(0), type(_type), size(_size), insensitive(0) {
	id = OptionParserIDGenerator::instance().next();
	s1 = new int8_t[size];
	memcpy(s1, list, size);
}
/* ------------------------------------------------------------------- */
OptionValidator::OptionValidator(char _type, char _op, const int8_t *list, int _size) : u1(0), op(_op), quiet(0), type(_type), size(_size), insensitive(0) {
	id = OptionParserIDGenerator::instance().next();
	u1 = new uint8_t[size];
	memcpy(u1, list, size);
}
/* ------------------------------------------------------------------- */
OptionValidator::OptionValidator(char _type, char _op, const int16_t *list, int _size) : s2(0), op(_op), quiet(0), type(_type), size(_size), insensitive(0) {
	id = OptionParserIDGenerator::instance().next();
	s2 = new int16_t[size];
	memcpy(s2, list, size*sizeof(int16_t));
}
/* ------------------------------------------------------------------- */
OptionValidator::OptionValidator(char _type, char _op, const uint16_t *list, int _size) : u2(0), op(_op), quiet(0), type(_type), size(_size), insensitive(0) {
	id = OptionParserIDGenerator::instance().next();
	u2 = new uint16_t[size];
	memcpy(u2, list, size*sizeof(uint16_t));
}
/* ------------------------------------------------------------------- */
OptionValidator::OptionValidator(char _type, char _op, const int32_t* list, int _size) : s4(0), op(_op), quiet(0), type(_type), size(_size), insensitive(0) {
	id = OptionParserIDGenerator::instance().next();
	s4 = new int32_t[size];
	memcpy(s4, list, size*sizeof(int32_t));
}
/* ------------------------------------------------------------------- */
OptionValidator::OptionValidator(char _type, char _op, const uint32_t *list, int _size) : u4(0), op(_op), quiet(0), type(_type), size(_size), insensitive(0) {
	id = OptionParserIDGenerator::instance().next();
	u4 = new uint32_t[size];
	memcpy(u4, list, size*sizeof(uint32_t));
}
/* ------------------------------------------------------------------- */
OptionValidator::OptionValidator(char _type, char _op, const int64_t *list, int _size) : s8(0), op(_op), quiet(0), type(_type), size(_size), insensitive(0) {
	id = OptionParserIDGenerator::instance().next();
	s8 = new int64_t[size];
	memcpy(s8, list, size*sizeof(int64_t));
}
/* ------------------------------------------------------------------- */
OptionValidator::OptionValidator(char _type, char _op, const uint64_t *list, int _size) : u8(0), op(_op), quiet(0), type(_type), size(_size), insensitive(0) {
	id = OptionParserIDGenerator::instance().next();
	u8 = new uint64_t[size];
	memcpy(u8, list, size*sizeof(uint64_t));
}
/* ------------------------------------------------------------------- */
OptionValidator::OptionValidator(char _type, char _op, const float* list, int _size) : f(0), op(_op), quiet(0), type(_type), size(_size), insensitive(0) {
	id = OptionParserIDGenerator::instance().next();
	f = new float[size];
	memcpy(f, list, size*sizeof(float));
}
/* ------------------------------------------------------------------- */
OptionValidator::OptionValidator(char _type, char _op, const double* list, int _size) : d(0), op(_op), quiet(0), type(_type), size(_size), insensitive(0) {
	id = OptionParserIDGenerator::instance().next();
	d = new double[size];
	memcpy(d, list, size*sizeof(double));
}
/* ------------------------------------------------------------------- */
OptionValidator::OptionValidator(char _type, char _op, const char** list, int _size, bool _insensitive) : t(0), op(_op), quiet(0), type(_type), size(_size), insensitive(_insensitive) {
	id = OptionParserIDGenerator::instance().next();
	t = new tinystl::string*[size];
	int i=0;

	for(; i < size; ++i) {
		t[i] = new tinystl::string(list[i]);
	}
}

/* ------------------------------------------------------------------- */
/* Less efficient but convenient ctor that parses strings to setup validator.
_type: s1, u1, s2, u2, ..., f, d, t
_op: lt, gt, ..., in
_list: comma-delimited string
*/
OptionValidator::OptionValidator(const char* _type, const char* _op, const char* _list, bool _insensitive) : t(0), quiet(0), type(0), size(0), insensitive(_insensitive) {
	id = OptionParserIDGenerator::instance().next();

	switch(_type[0]) {
	case 'u':
		switch(_type[1]) {
		case '1': type = U1; break;
		case '2': type = U2; break;
		case '4': type = U4; break;
		case '8': type = U8; break;
		default: break;
		}
		break;
	case 's':
		switch(_type[1]) {
		case '1': type = S1; break;
		case '2': type = S2; break;
		case '4': type = S4; break;
		case '8': type = S8; break;
		default: break;
		}
		break;
	case 'f': type = F; break;
	case 'd': type = D; break;
	case 't': type = T; break;
	default:
		if (!quiet)
			LOGERRORF("ERROR: Unknown validator datatype \"%s\".\n", _type);
		break;
	}

	int nop = 0;
	if (_op != 0)
		nop = strlen(_op);

	switch(nop) {
	case 0: op = NOOP; break;
	case 2:
		switch(_op[0]) {
		case 'g':
			switch(_op[1]) {
			case 'e': op = GE; break;
			default: op = GT; break;
			}
			break;
		case 'i': op = IN;
			break;
		default:
			switch(_op[1]) {
			case 'e': op = LE; break;
			default: op = LT; break;
			}
			break;
		}
		break;
	case 4:
		switch(_op[1]) {
		case 'e':
			switch(_op[3]) {
			case 'e': op = GELE; break;
			default: op = GELT; break;
			}
			break;
		default:
			switch(_op[3]) {
			case 'e': op = GTLE; break;
			default: op = GTLT; break;
			}
			break;
		}
		break;
	default:
		if (!quiet)
			LOGERRORF("ERROR: Unknown validator operation \"%s\".\n", _op);
		break;
	}

	if (_list == 0) return;
	// Create list of strings and then cast to native datatypes.
	tinystl::string unsplit(_list);
	tinystl::vector<tinystl::string*> split;
	tinystl::vector<tinystl::string*>::iterator it;
	SplitDelim(unsplit, ',', &split);
	size = split.size();
	tinystl::string **strings = new tinystl::string*[size];

	int i = 0;
	for(it = split.begin(); it != split.end(); ++it)
		strings[i++] = *it;

	if (insensitive)
		for(i=0; i < size; ++i)
			ToLowerASCII(*strings[i]);

#define FreeStrings() { \
    for(i=0; i < size; ++i)\
      delete strings[i];\
    delete [] strings;\
  }

#define ToArray(T,P,Y) case T: P = new Y[size]; To##T(strings, P, size); FreeStrings(); break;
	switch(type) {
	ToArray(S1,s1,int8_t);
	ToArray(U1,u1,uint8_t);
	ToArray(S2,s2,int16_t);
	ToArray(U2,u2,uint16_t);
	ToArray(S4,s4,int32_t);
	ToArray(U4,u4,uint32_t);
	ToArray(S8,s8,int64_t);
	ToArray(U8,u8,uint64_t);
	ToArray(F,f,float);
	ToArray(D,d,double);
	case T: t = strings; break; /* Don't erase strings array. */
	default: break;
	}
};
/* ------------------------------------------------------------------- */
void OptionValidator::print() {
	LOGINFOF("id=%d, op=%d, type=%d, size=%d, insensitive=%d\n", id, op, type, size, insensitive);
};

/* ------------------------------------------------------------------- */
bool OptionValidator::isValid(const tinystl::string * valueAsString) {
#define CHECKRANGE(E,T) {\
  int64_t E##value = (int64_t)strtoll(valueAsString->c_str(), NULL, 0); \
  int64_t E##min = static_cast<int64_t>(std::numeric_limits<T>::min()); \
  if (E##value < E##min) { \
    if (!quiet) \
      LOGERRORF("ERROR: Invalid value %i is less than datatype min %i.\n", E##value, E##min); \
    return false; \
  } \
  \
  long long E##max = static_cast<long long>(std::numeric_limits<T>::max()); \
  if (E##value > E##max) { \
    if (!quiet) \
      LOGERRORF("ERROR: Invalid value %i is greater than datatype max %i.\n", E##value, E##max); \
    return false; \
  } \
}
	// Check if within datatype limits.
	if (valueAsString == 0) return false;
	if (type != T) {
		switch(type) {
		case S1: CHECKRANGE(S1,int8_t); break;
		case U1: CHECKRANGE(U1,uint8_t); break;
		case S2: CHECKRANGE(S2,int16_t); break;
		case U2: CHECKRANGE(U2,uint16_t); break;
		case S4: CHECKRANGE(S4,int32_t); break;
		case U4: CHECKRANGE(U4,uint32_t); break;
		case S8: CHECKRANGE(S8,int64_t); break;
//		case U8: CHECKRANGE(U8,uint64_t); break;

		case F: {
			double dmax = static_cast<double>(std::numeric_limits<float>::max());
			double dvalue = atof(valueAsString->c_str());
			double dmin = -dmax;
			if (dvalue < dmin) {
				if (!quiet) {
					LOGERRORF("ERROR: Invalid value %g is less than datatype min %g.\n", dvalue, dmin);
				}
				return false;
			}

			if (dvalue > dmax) {
				if (!quiet)
					LOGERRORF("ERROR: Invalid value %g is greater than datatype max %g.\n", dvalue, dmax);
				return false;
			}
		} break;

		default: // uint64_t and double don't range check
		case NOTYPE: break;
		}
	} else {
		if (op == IN) {
			int i=0;
			if (insensitive) {
				tinystl::string valueAsStringLower(*valueAsString);
				ToLowerASCII(valueAsStringLower);
				for(; i < size; ++i) {
					if (valueAsStringLower.compare(t[i]->c_str()) == 0)
						return true;
				}
			} else {
				for(; i < size; ++i) {
					if (valueAsString->compare(t[i]->c_str()) == 0)
						return true;
				}
			}
			return false;
		}
	}


	// Only check datatype limits, and return;
	if (op == NOOP) return true;

#define VALIDATE(T, U, LIST) { \
  /* Value string converted to true native type. */ \
  U v = (U)strtoll(valueAsString->c_str(), NULL, 0);\
  /* Check if within list. */ \
  if (op == IN) { \
  	for(T* cur = LIST; cur < LIST + size;++cur) if(*cur == v) return true; \
  	return false; \
  } \
  \
  /* Check if within user's custom range. */ \
  T v0, v1; \
  if (size > 0) { \
    v0 = LIST[0]; \
  } \
  \
  if (size > 1) { \
    v1 = LIST[1]; \
  } \
  \
  switch (op) {\
    case LT:\
      if (size > 0) {\
        return v < v0;\
      } else {\
        LOGERRORF("ERROR: No value given to validate if %i < X.\n", v);\
        return false;\
      }\
      break;\
    case LE:\
      if (size > 0) {\
        return v <= v0;\
      } else {\
        LOGERRORF("ERROR: No value given to validate if %i <= X.\n", v);\
        return false;\
      }\
      break;\
    case GT:\
      if (size > 0) {\
        return v > v0;\
      } else {\
        LOGERRORF("ERROR: No value given to validate if %i > X.\n", v);\
        return false;\
      }\
      break;\
    case GE:\
      if (size > 0) {\
        return v >= v0;\
      } else {\
        LOGERRORF("ERROR: No value given to validate if %i >= X.\n", v);\
        return false;\
      }\
      break;\
    case GTLT:\
      if (size > 1) {\
        return (v0 < v) && (v < v1);\
      } else {\
        LOGERRORF("ERROR: Missing values given to validate if X1 < %i < X2.\n", v);\
        return false;\
      }\
      break;\
    case GELT:\
      if (size > 1) {\
        return (v0 <= v) && (v < v1);\
      } else {\
        LOGERRORF("ERROR: Missing values given to validate if X1 <= %i < X.\n", v);\
        return false;\
      }\
      break;\
    case GELE:\
      if (size > 1) {\
        return (v0 <= v) && (v <= v1);\
      } else {\
        LOGERRORF("ERROR: Missing values given to validate if X1 <= %i <= X2.\n", v);\
        return false;\
      }\
      break;\
    case GTLE:\
      if (size > 1) {\
        return (v0 < v) && (v <= v1);\
      } else {\
        LOGERRORF("ERROR: Missing values given to validate if X1 < %i <= X2.\n", v);\
        return false;\
      }\
      break;\
      case NOOP: case IN: default: break;\
  } \
  }

	switch(type) {
	case U1: VALIDATE(uint8_t, int, u1); break;
	case S1: VALIDATE(int8_t, int, s1); break;
	case U2: VALIDATE(uint16_t, int, u2); break;
	case S2: VALIDATE(int16_t, int, s2); break;
	case U4: VALIDATE(uint32_t, unsigned int, u4); break;
	case S4: VALIDATE(int32_t, int, s4); break;
	case U8: VALIDATE(uint64_t, int64_t , u8); break;
	case S8: VALIDATE(int64_t, uint64_t , s8); break;
	case F: VALIDATE(float, float, f); break;
	case D: VALIDATE(double, double, d); break;
	default: break;
	}

	return true;
};

void OptionGroup::clearArgs() {
	int i,j;
	for(i=0; i < (long int)args.size(); ++i) {
		for(j=0; j < (long int)args[i]->size(); ++j)
			delete args[i]->at(j);

		delete args[i];
	}

	args.clear();
	isSet = false;
};

void OptionGroup::getInt64(int64_t & out) {
	if (!isSet) {
		if (defaults.empty())
			out = 0;
		else {
			out = strtoll(defaults.c_str(), NULL, 0);
		}
	} else {
		if (args.empty() || args[0]->empty())
			out = 0;
		else {
			out = strtoll(args[0]->at(0)->c_str(), NULL, 0);
		}
	}
};

void OptionGroup::getFloat(float & out) {
	if (!isSet) {
		if (defaults.empty())
			out = 0.0;
		else
			out = (float)atof(defaults.c_str());
	} else {
		if (args.empty() || args[0]->empty())
			out = 0.0;
		else {
			out = (float)atof(args[0]->at(0)->c_str());
		}
	}
};

void OptionGroup::getDouble(double & out) {
	if (!isSet) {
		if (defaults.empty())
			out = 0.0;
		else
			out = atof(defaults.c_str());
	} else {
		if (args.empty() || args[0]->empty())
			out = 0.0;
		else {
			out = atof(args[0]->at(0)->c_str());
		}
	}
}

void OptionGroup::getString(tinystl::string & out) {
	if (!isSet) {
		out = defaults;
	} else {
		if (args.empty() || args[0]->empty())
			out = "";
		else {
			out = *args[0]->at(0);
		}
	}
}

void OptionGroup::getInt64s(tinystl::vector<int64_t> & out) {
	if (!isSet) {
		if (!defaults.empty()) {
			tinystl::vector< tinystl::string > strings;
			SplitDelim(defaults, delim, strings);
			StringsToInt64s(strings, out);
		}
	} else {
		if (!(args.empty() || args[0]->empty()))
			StringsToInt64s(args[0], &out);
	}
}

void OptionGroup::getFloats(tinystl::vector<float> & out) {
	if (!isSet) {
		if (!defaults.empty()) {
			tinystl::vector< tinystl::string > strings;
			SplitDelim(defaults, delim, strings);
			StringsToFloats(strings, out);
		}
	} else {
		if (!(args.empty() || args[0]->empty()))
			StringsToFloats(args[0], &out);
	}
}

void OptionGroup::getDoubles(tinystl::vector<double> & out) {
	if (!isSet) {
		if (!defaults.empty()) {
			tinystl::vector< tinystl::string > strings;
			SplitDelim(defaults, delim, strings);
			StringsToDoubles(strings, out);
		}
	} else {
		if (!(args.empty() || args[0]->empty()))
			StringsToDoubles(args[0], &out);
	}
}

void OptionGroup::getStrings(tinystl::vector<tinystl::string>& out) {
	if (!isSet) {
		if (!defaults.empty()) {
			SplitDelim(defaults, delim, out);
		}
	} else {
		if (!(args.empty() || args[0]->empty()))
			StringsToStrings(args[0], &out);
	}
}

void OptionGroup::getMultiInt64s(tinystl::vector< tinystl::vector<int64_t> >& out) {
	if (!isSet) {
		if (!defaults.empty()) {
			tinystl::vector< tinystl::string > strings;
			SplitDelim(defaults, delim, strings);
			if (out.size() < 1) out.resize(1);
			StringsToInt64s(strings, out[0]);
		}
	} else {
		if (!args.empty()) {
			int n = args.size();
			if ((long int)out.size() < n) out.resize(n);
			for(int i=0; i < n; ++i) {
				StringsToInt64s(args[i], &out[i]);
			}
		}
	}
}

void OptionGroup::getMultiFloats(tinystl::vector< tinystl::vector<float> >& out) {
	if (!isSet) {
		if (!defaults.empty()) {
			tinystl::vector< tinystl::string > strings;
			SplitDelim(defaults, delim, strings);
			if (out.size() < 1) out.resize(1);
			StringsToFloats(strings, out[0]);
		}
	} else {
		if (!args.empty()) {
			int n = args.size();
			if ((long int)out.size() < n) out.resize(n);
			for(int i=0; i < n; ++i) {
				StringsToFloats(args[i], &out[i]);
			}
		}
	}
}

void OptionGroup::getMultiDoubles(tinystl::vector< tinystl::vector<double> >& out) {
	if (!isSet) {
		if (!defaults.empty()) {
			tinystl::vector< tinystl::string > strings;
			SplitDelim(defaults, delim, strings);
			if (out.size() < 1) out.resize(1);
			StringsToDoubles(strings, out[0]);
		}
	} else {
		if (!args.empty()) {
			int n = args.size();
			if ((long int)out.size() < n) out.resize(n);
			for(int i=0; i < n; ++i) {
				StringsToDoubles(args[i], &out[i]);
			}
		}
	}
}


void OptionGroup::getMultiStrings(tinystl::vector< tinystl::vector<tinystl::string> >& out) {
	if (!isSet) {
		if (!defaults.empty()) {
			tinystl::vector< tinystl::string > strings;
			SplitDelim(defaults, delim, strings);
			if (out.size() < 1) out.resize(1);
			out[0] = strings;
		}
	} else {
		if (!args.empty()) {
			int n = args.size();
			if ((long int)out.size() < n) out.resize(n);

			for(int i=0; i < n; ++i) {
				for(int j=0; j < (long int)args[i]->size(); ++j)
					out[i].push_back( *args[i]->at(j) );
			}
		}
	};
}

void OptionParser::reset() {
	this->doublespace = 1;

	int i;
	for(i=0; i < (long int)groups.size(); ++i)
		delete groups[i];
	groups.clear();

	for(i=0; i < (long int)unknownArgs.size(); ++i)
		delete unknownArgs[i];
	unknownArgs.clear();

	for(i=0; i < (long int)firstArgs.size(); ++i)
		delete firstArgs[i];
	firstArgs.clear();

	for(i=0; i < (long int)lastArgs.size(); ++i)
		delete lastArgs[i];
	lastArgs.clear();

	ValidatorMap::iterator it;
	for(it = validators.begin(); it != validators.end(); ++it)
		delete it->second;

	validators.clear();
	optionGroupIds.clear();
	groupValidators.clear();
};

OptionParser::~OptionParser() {
	reset();
}

void OptionParser::resetArgs() {
	int i;
	for(i=0; i < (long int)groups.size(); ++i)
		groups[i]->clearArgs();

	for(i=0; i < (long int)unknownArgs.size(); ++i)
		delete unknownArgs[i];
	unknownArgs.clear();

	for(i=0; i < (long int)firstArgs.size(); ++i)
		delete firstArgs[i];
	firstArgs.clear();

	for(i=0; i < (long int)lastArgs.size(); ++i)
		delete lastArgs[i];
	lastArgs.clear();
}

void OptionParser::add(const char * defaults, bool required, int expectArgs, char delim, const char * help, const char * flag1, OptionValidator* validator) {
	int id = this->groups.size();
	OptionGroup * g = new OptionGroup;
	g->defaults = defaults;
	g->isRequired = required;
	g->expectArgs = expectArgs;
	g->delim = delim;
	g->isSet = 0;
	g->help = help;
	tinystl::string *f1 = new tinystl::string(flag1);
	g->flags.push_back( f1 );
	this->optionGroupIds[flag1] = id;
	this->groups.push_back(g);

	if (validator) {
		int vid = validator->id;
		validators[vid] = validator;
		groupValidators[id] = vid;
	} else {
		groupValidators[id] = -1;
	}
}


void OptionParser::add(const char * defaults, bool required, int expectArgs, char delim, const char * help, const char * flag1, const char * flag2, OptionValidator* validator) {
	int id = this->groups.size();
	OptionGroup * g = new OptionGroup;
	g->defaults = defaults;
	g->isRequired = required;
	g->expectArgs = expectArgs;
	g->delim = delim;
	g->isSet = 0;
	g->help = help;
	tinystl::string *f1 = new tinystl::string(flag1);
	g->flags.push_back( f1 );
	tinystl::string *f2 = new tinystl::string(flag2);
	g->flags.push_back( f2 );
	this->optionGroupIds[flag1] = id;
	this->optionGroupIds[flag2] = id;

	this->groups.push_back(g);

	if (validator) {
		int vid = validator->id;
		validators[vid] = validator;
		groupValidators[id] = vid;
	} else {
		groupValidators[id] = -1;
	}
}


void OptionParser::add(const char * defaults, bool required, int expectArgs, char delim, const char * help, const char * flag1, const char * flag2, const char * flag3, OptionValidator* validator) {
	int id = this->groups.size();
	OptionGroup * g = new OptionGroup;
	g->defaults = defaults;
	g->isRequired = required;
	g->expectArgs = expectArgs;
	g->delim = delim;
	g->isSet = 0;
	g->help = help;
	tinystl::string *f1 = new tinystl::string(flag1);
	g->flags.push_back( f1 );
	tinystl::string *f2 = new tinystl::string(flag2);
	g->flags.push_back( f2 );
	tinystl::string *f3 = new tinystl::string(flag3);
	g->flags.push_back( f3 );
	this->optionGroupIds[flag1] = id;
	this->optionGroupIds[flag2] = id;
	this->optionGroupIds[flag3] = id;

	this->groups.push_back(g);

	if (validator) {
		int vid = validator->id;
		validators[vid] = validator;
		groupValidators[id] = vid;
	} else {
		groupValidators[id] = -1;
	}
}

void OptionParser::add(const char * defaults, bool required, int expectArgs, char delim, const char * help, const char * flag1, const char * flag2, const char * flag3, const char * flag4, OptionValidator* validator) {
	int id = this->groups.size();
	OptionGroup * g = new OptionGroup;
	g->defaults = defaults;
	g->isRequired = required;
	g->expectArgs = expectArgs;
	g->delim = delim;
	g->isSet = 0;
	g->help = help;
	tinystl::string *f1 = new tinystl::string(flag1);
	g->flags.push_back( f1 );
	tinystl::string *f2 = new tinystl::string(flag2);
	g->flags.push_back( f2 );
	tinystl::string *f3 = new tinystl::string(flag3);
	g->flags.push_back( f3 );
	tinystl::string *f4 = new tinystl::string(flag4);
	g->flags.push_back( f4 );
	this->optionGroupIds[flag1] = id;
	this->optionGroupIds[flag2] = id;
	this->optionGroupIds[flag3] = id;
	this->optionGroupIds[flag4] = id;

	this->groups.push_back(g);

	if (validator) {
		int vid = validator->id;
		validators[vid] = validator;
		groupValidators[id] = vid;
	} else {
		groupValidators[id] = -1;
	}
}

bool OptionParser::exportFile(VFile_Handle handle, bool all) {
	int i;
	tinystl::string out;
	bool quote;

	// Export the first args, except the program name, so start from 1.
	for(i=1; i < (long int)firstArgs.size(); ++i) {
		quote = ((firstArgs[i]->find_first_of(" \t") != tinystl::string::npos) && (firstArgs[i]->find_first_of("\'\"") == tinystl::string::npos));

		if (quote)
			out.append("\"");

		out.append(*firstArgs[i]);
		if (quote)
			out.append("\"");

		out.append(" ");
	}

	if (firstArgs.size() > 1)
		out.append("\n");

	tinystl::vector<tinystl::string* > stringPtrs(groups.size());
	int m;
	int n = groups.size();
	for(i=0; i < n; ++i) {
		stringPtrs[i] = groups[i]->flags[0];
	}

	OptionGroup *g;
	// Sort first flag of each group with other groups.
	std::sort(stringPtrs.begin(), stringPtrs.end(), CmpOptStringPtr);
	for(i=0; i < n; ++i) {
		g = get(stringPtrs[i]->c_str());
		if (g->isSet || all) {
			if (!g->isSet || g->args.empty()) {
				if (!g->defaults.empty()) {
					out.append(*stringPtrs[i]);
					out.append(" ");
					quote = ((g->defaults.find_first_of(" \t") != tinystl::string::npos) && (g->defaults.find_first_of("\'\"") == tinystl::string::npos));
					if (quote)
						out.append("\"");

					out.append(g->defaults);
					if (quote)
						out.append("\"");

					out.append("\n");
				}
			} else {
				int n = g->args.size();
				for(int j=0; j < n; ++j) {
					out.append(*stringPtrs[i]);
					out.append(" ");
					m = g->args[j]->size();

					for(int k=0; k < m; ++k) {
						quote = ( (*g->args[j]->at(k)).find_first_of(" \t") != tinystl::string::npos );
						if (quote)
							out.append("\"");

						out.append(*g->args[j]->at(k));
						if (quote)
							out.append("\"");

						if ((g->delim) && ((k+1) != m))
							out.append(1,g->delim);
					}
					out.append("\n");
				}
			}
		}
	}

	// Export the last args.
	for(i=0; i < (long int)lastArgs.size(); ++i) {
		quote = ( lastArgs[i]->find_first_of(" \t") != tinystl::string::npos );
		if (quote)
			out.append("\"");

		out.append(*lastArgs[i]);
		if (quote)
			out.append("\"");

		out.append(" ");
	}

	VFile::File* file = VFile::File::FromHandle(handle);
	file->Write(out.c_str(), out.size()+1);

	return true;
};

// Does not overwrite current options.
// Returns true if file was read successfully.
// So if this is used before parsing CLI, then option values will reflect
// this file, but if used after parsing CLI, then values will contain
// both CLI values and file's values.
//
// Comment lines are allowed if prefixed with #.
// Strings should be quoted as usual.

bool OptionParser::importFile(VFile_Handle handle, char comment) {
	VFile::File* file = VFile::File::FromHandle(handle);

	tinystl::string memblockstring;
	// Read entire file contents.
	if(file->GetType() == VFile_Type_Memory) {
		VFile_MemFile_t* minterface = (VFile_MemFile_t*) file->GetTypeSpecificData();
		memblockstring = tinystl::string((char*)minterface->memory);
	} else {
		size_t size = file->Size();
		char* memblock = (char*)MEMORY_TEMP_MALLOC(size+1); // Add one for end of string.
		file->Read(memblock, size);
		memblock[size] = '\0';
		memblockstring = tinystl::string(memblock);
		MEMORY_TEMP_FREE(memblock);
	}

	// Find comment lines.
	tinystl::vector<tinystl::string*> lines;
	SplitDelim(memblockstring, '\n', &lines);
	int i,j,n;
	tinystl::vector<tinystl::string*>::iterator iter;
	tinystl::vector<int> sq, dq; // Single and double quote indices.
	tinystl::vector<int>::iterator lo; // For searching quote indices.
	size_t pos;
	const char *str;
	tinystl::string *line;
	// Find all single and double quotes to correctly handle comment tokens.
	for(iter=lines.begin(); iter != lines.end(); ++iter) {
		line = *iter;
		str = line->c_str();
		n = line->size();
		sq.clear();
		dq.clear();
		if (n) {
			// If first char is comment, then erase line and continue.
			pos = line->find_first_not_of(" \t\r");
			if ((pos==tinystl::string::npos) || (line->at(pos)==comment)) {
				line->resize(0);
				continue;
			} else {
				// Erase whitespace prefix.
				line->erase(0,pos);
				n = line->size();
			}

			if (line->at(0)=='"')
				dq.push_back(0);

			if (line->at(0)=='\'')
				sq.push_back(0);
		} else { // Empty line.
			continue;
		}

		for(i=1; i < n; ++i) {
			if ( (str[i]=='"') && (str[i-1]!='\\') )
				dq.push_back(i);
			else if ( (str[i]=='\'') && (str[i-1]!='\\') )
				sq.push_back(i);
		}
		// Scan for comments, and when found, check bounds of quotes.
		// Start with second char because already checked first char.
		for(i=1; i < n; ++i) {
			if ( (line->at(i)==comment) && (line->at(i-1)!='\\') ) {
				// If within open/close quote pair, then not real comment.
				if (sq.size()) {
					lo = std::lower_bound(sq.begin(), sq.end(), i);
					// All start of strings will be even indices, closing quotes is odd indices.
					j = (int)(lo-sq.begin());
					if ( (j % 2) == 0) { // Even implies comment char not in quote pair.
						// Erase from comment char to end of line.
						line->erase(i);
						break;
					}
				}  else if (dq.size()) {
					// Repeat tests for double quotes.
					lo = std::lower_bound(dq.begin(), dq.end(), i);
					j = (int)(lo-dq.begin());
					if ( (j % 2) == 0) {
						line->erase(i);
						break;
					}
				} else {
					// Not in quotes.
					line->erase(i);
					break;
				}
			}
		}
	}

	tinystl::string cmd;
	// Convert list to string without newlines to simulate commandline.
	for(iter=lines.begin(); iter != lines.end(); ++iter) {
		if (! (*iter)->empty()) {
			cmd.append(**iter);
			cmd.append(" ");
		}
	}

	// Now parse as if from command line.
	int argc=0;
	char** argv = CommandLineToArgvA((char*)cmd.c_str(), &argc);

	// Parse.
	parse(argc, (const char**)argv);
	if (argv) free(argv);
	for(iter=lines.begin(); iter != lines.end(); ++iter)
		delete *iter;

	return true;
};

int OptionParser::isSet(const char * name) {
	tinystl::string sname(name);

	if (this->optionGroupIds.find(sname) != this->optionGroupIds.end()) {
		return this->groups[this->optionGroupIds[sname]]->isSet;
	}

	return 0;
};

int OptionParser::isSet(tinystl::string & name) {
	if (this->optionGroupIds.find(name) != this->optionGroupIds.end()) {
		return this->groups[this->optionGroupIds[name]]->isSet;
	}

	return 0;
};

OptionGroup * OptionParser::get(const char * name) {
	if (this->optionGroupIds.find(name) != this->optionGroupIds.end()) {
		return groups[optionGroupIds[name]];
	}

	return nullptr;
};

void OptionParser::getUsage(tinystl::string & usage, int width, Layout layout) {

	usage.append(overview);
	usage.append("\n\n");
	usage.append("USAGE: ");
	usage.append(syntax);
	usage.append("\n\nOPTIONS:\n\n");
	getUsageDescriptions(usage, width, layout);

	if (!example.empty()) {
		usage.append("EXAMPLES:\n\n");
		usage.append(example);
	}

	if (!footer.empty()) {
		usage.append(footer);
	}
};

// Creates 2 column formatted help descriptions for each option flag.
void OptionParser::getUsageDescriptions(tinystl::string & usage, int width, Layout layout) {
	// Sort each flag list amongst each group.
	int i;
	// Store index of flag groups before sort for easy lookup later.
	tinystl::unordered_map<tinystl::string*, int> stringPtrToIndexMap;
	tinystl::vector<tinystl::string* > stringPtrs(groups.size());

	for(i=0; i < (long int)groups.size(); ++i) {
		std::sort(groups[i]->flags.begin(), groups[i]->flags.end(), CmpOptStringPtr);
		stringPtrToIndexMap[groups[i]->flags[0]] = i;
		stringPtrs[i] = groups[i]->flags[0];
	}

	size_t j, k;
	tinystl::string opts;
	tinystl::vector<tinystl::string> sortedOpts;
	// Sort first flag of each group with other groups.
	std::sort(stringPtrs.begin(), stringPtrs.end(), CmpOptStringPtr);
	for(i=0; i < (long int)groups.size(); ++i) {
		//printf("DEBUG:%d: %d %d %s\n", __LINE__, i, stringPtrToIndexMap[stringPtrs[i]], stringPtrs[i]->c_str());
		k = stringPtrToIndexMap[stringPtrs[i]];
		opts.clear();
		for(j=0; j < groups[k]->flags.size()-1; ++j) {
			opts.append(*groups[k]->flags[j]);
			opts.append(", ");

			if ((long int)opts.size() > width)
				opts.append("\n");
		}
		// The last flag. No need to append comma anymore.
		opts.append( *groups[k]->flags[j] );

		if (groups[k]->expectArgs) {
			opts.append(" ARG");

			if (groups[k]->delim) {
				opts.append("1[");
				opts.append(1, groups[k]->delim);
				opts.append("ARGn]");
			}
		}

		sortedOpts.push_back(opts);
	}

	// Each option group will use this to build multiline help description.
	tinystl::vector<tinystl::string*> desc;
	// Number of whitespaces from start of line to description (interleave layout) or
	// gap between flag names and description (align, stagger layouts).
	int gutter = 3;

	// Find longest opt flag string to set column start for help usage descriptions.
	int maxlen=0;
	if (layout == ALIGN) {
		for(i=0; i < (long int)groups.size(); ++i) {
			if (maxlen < (long int)sortedOpts[i].size())
				maxlen = sortedOpts[i].size();
		}
	}

	// The amount of space remaining on a line for help text after flags.
	int helpwidth;
	tinystl::vector<tinystl::string*>::iterator cIter, insertionIter;
	size_t pos;
	for(i=0; i < (long int)groups.size(); ++i) {
		k = stringPtrToIndexMap[stringPtrs[i]];

		if (layout == STAGGER)
			maxlen = sortedOpts[i].size();

		int pad = gutter + maxlen;
		helpwidth = width - pad;

		// All the following split-fu could be optimized by just using substring (offset, length) tuples, but just to get it done, we'll do some not-too expensive string copying.
		SplitDelim(groups[k]->help, '\n', &desc);
		// Split lines longer than allowable help width.
		for(insertionIter=desc.begin(), cIter=insertionIter++;
				cIter != desc.end();
				cIter=insertionIter++) {
			if ((long int)((*cIter)->size()) > helpwidth) {
				// Get pointer to next string to insert new strings before it.
				tinystl::string *rem = *cIter;
				// Remove this line and add back in pieces.
				desc.erase(cIter);
				// Loop until remaining string is short enough.
				while ((long int)rem->size() > helpwidth) {
					// Find whitespace to split before helpwidth.
					if (rem->at(helpwidth) == ' ') {
						// If word ends exactly at helpwidth, then split after it.
						pos = helpwidth;
					} else {
						// Otherwise, split occurs midword, so find whitespace before this word.
						pos = rem->rfind(' ', helpwidth);
					}
					// Insert split string.
					desc.insert(insertionIter, new tinystl::string(*rem, 0, pos));
					// Now skip any whitespace to start new line.
					pos = rem->find_first_not_of(" ", pos);
					rem->erase(0, pos);
				}

				if (rem->size())
					desc.insert(insertionIter, rem);
				else
					delete rem;
			}
		}

		usage.append(sortedOpts[i]);
		if (layout != INTERLEAVE)
			// Add whitespace between option names and description.
			usage.append(pad - sortedOpts[i].size(), ' ');
		else {
			usage.append("\n");
			usage.append(gutter, ' ');
		}

		if (desc.size() > 0) { // Crash fix by Bruce Shankle.
			// First line already padded above (before calling SplitDelim) after option flag names.
			cIter = desc.begin();
			usage.append(**cIter);
			usage.append("\n");
			// Now inject the pad for each line.
			for(++cIter; cIter != desc.end(); ++cIter) {
				usage.append(pad, ' ');
				usage.append(**cIter);
				usage.append("\n");
			}

			if (this->doublespace) usage.append("\n");

			for(cIter=desc.begin(); cIter != desc.end(); ++cIter)
				delete *cIter;

			desc.clear();
		}

	}
};
/* ################################################################### */
bool OptionParser::gotExpected(tinystl::vector<tinystl::string> & badOptions) {
	int i,j;

	for(i=0; i < (long int)groups.size(); ++i) {
		OptionGroup *g = groups[i];
		// If was set, ensure number of args is correct.
		if (g->isSet) {
			if ((g->expectArgs != 0) && g->args.empty()) {
				badOptions.push_back(*g->flags[0]);
				continue;
			}

			for(j=0; j < (long int)g->args.size(); ++j) {
				if ((g->expectArgs != -1) && (g->expectArgs != (long int)g->args[j]->size()))
					badOptions.push_back(*g->flags[0]);
			}
		}
	}

	return badOptions.empty();
};
/* ################################################################### */
bool OptionParser::gotRequired(tinystl::vector<tinystl::string> & badOptions) {
	int i;

	for(i=0; i < (long int)groups.size(); ++i) {
		OptionGroup *g = groups[i];
		// Simple case when required but user never set it.
		if (g->isRequired && (!g->isSet)) {
			badOptions.push_back(*g->flags[0]);
			continue;
		}
	}

	return badOptions.empty();
};
/* ################################################################### */
bool OptionParser::gotValid(tinystl::vector<tinystl::string> & badOptions, tinystl::vector<tinystl::string> & badArgs) {
	int groupid, validatorid;
	tinystl::unordered_map< int, int >::iterator it;

	for(it = groupValidators.begin(); it != groupValidators.end(); ++it) {
		groupid = it->first;
		validatorid = it->second;
		if (validatorid < 0) continue;

		OptionGroup *g = groups[groupid];
		OptionValidator *v = validators[validatorid];
		bool nextgroup = false;

		for (int i = 0; i < (long int)g->args.size(); ++i) {
			if (nextgroup) break;
			tinystl::vector< tinystl::string* > * args = g->args[i];
			for (int j = 0; j < (long int)args->size(); ++j) {
				if (!v->isValid(args->at(j))) {
					badOptions.push_back(*g->flags[0]);
					badArgs.push_back(*args->at(j));
					nextgroup = true;
					break;
				}
			}
		}
	}

	return badOptions.empty();
};
/* ################################################################### */
void OptionParser::parse(int argc, const char * argv[]) {
	if (argc < 1) return;

	/*
	tinystl::unordered_map<tinystl::string,int>::iterator it;
	for ( it=optionGroupIds.begin() ; it != optionGroupIds.end(); it++ )
		std::cout << (*it).first << " => " << (*it).second << std::endl;
	*/

	int i, k, firstOptIndex=0, lastOptIndex=0;
	tinystl::string s;
	OptionGroup *g;

	for(i=0; i < argc; ++i) {
		s = argv[i];

		if (optionGroupIds.find(s) != optionGroupIds.end())
			break;
	}

	firstOptIndex = i;

	if (firstOptIndex == argc) {
		// No flags encountered, so set last args.
		this->firstArgs.push_back(new tinystl::string(argv[0]));

		for(k=1; k < argc; ++k)
			this->lastArgs.push_back(new tinystl::string(argv[k]));

		return;
	}

	// Store initial args before opts appear.
	for(k=0; k < i; ++k) {
		this->firstArgs.push_back(new tinystl::string(argv[k]));
	}

	for(; i < argc; ++i) {
		s = argv[i];

		if (optionGroupIds.find(s) != optionGroupIds.end()) {
			k = optionGroupIds[s];
			g = groups[k];
			g->isSet = 1;
			g->parseIndex.push_back(i);

			if (g->expectArgs) {
				// Read ahead to get args.
				++i;
				if (i >= argc) return;
				g->args.push_back(new tinystl::vector<tinystl::string*>);
				SplitDelim(argv[i], g->delim, g->args.back());
			}
			lastOptIndex = i;
		}
	}

	// Scan for unknown opts/arguments.
	for(i=firstOptIndex; i <= lastOptIndex; ++i) {
		s = argv[i];

		if (optionGroupIds.find(s) != optionGroupIds.end()) {
			k = optionGroupIds[s];
			g = groups[k];
			if (g->expectArgs) {
				// Read ahead for args and skip them.
				++i;
			}
		} else {
			unknownArgs.push_back(new tinystl::string(argv[i]));
		}
	}

	if ( lastOptIndex >= (argc-1) ) return;

	// Store final args without flags.
	for(k=lastOptIndex + 1; k < argc; ++k) {
		this->lastArgs.push_back(new tinystl::string(argv[k]));
	}
};
/* ################################################################### */
void OptionParser::prettyPrint(tinystl::string & out) {
	char tmp[256];
	int i,j,k;

	out += "First Args:\n";
	for(i=0; i < (long int)firstArgs.size(); ++i) {
		sprintf(tmp, "%d: %s\n", i+1, firstArgs[i]->c_str());
		out += tmp;
	}

	// Sort the option flag names.
	int n = groups.size();
	tinystl::vector<tinystl::string* > stringPtrs(n);
	for(i=0; i < n; ++i) {
		stringPtrs[i] = groups[i]->flags[0];
	}

	// Sort first flag of each group with other groups.
	std::sort(stringPtrs.begin(), stringPtrs.end(), CmpOptStringPtr);

	out += "\nOptions:\n";
	OptionGroup *g;
	for(i=0; i < n; ++i) {
		g = get(stringPtrs[i]->c_str());
		out += "\n";
		// The flag names:
		for(j=0; j < (long int)g->flags.size()-1; ++j) {
			sprintf(tmp, "%s, ", g->flags[j]->c_str());
			out += tmp;
		}
		sprintf(tmp, "%s:\n", g->flags.back()->c_str());
		out += tmp;

		if (g->isSet) {
			if (g->expectArgs) {
				if (g->args.empty()) {
					sprintf(tmp, "%s (default)\n", g->defaults.c_str());
					out += tmp;
				} else {
					for(k=0; k < (long int)g->args.size(); ++k) {
						for(j=0; j < (long int)g->args[k]->size()-1; ++j) {
							sprintf(tmp, "%s%c", g->args[k]->at(j)->c_str(), g->delim);
							out += tmp;
						}
						sprintf(tmp, "%s\n", g->args[k]->back()->c_str());
						out += tmp;
					}
				}
			} else { // Set but no args expected.
				sprintf(tmp, "Set\n");
				out += tmp;
			}
		} else {
			sprintf(tmp, "Not set\n");
			out += tmp;
		}
	}

	out += "\nLast Args:\n";
	for(i=0; i < (long int)lastArgs.size(); ++i) {
		sprintf(tmp, "%d: %s\n", i+1, lastArgs[i]->c_str());
		out += tmp;
	}

	out += "\nUnknown Args:\n";
	for(i=0; i < (long int)unknownArgs.size(); ++i) {
		sprintf(tmp, "%d: %s\n", i+1, unknownArgs[i]->c_str());
		out += tmp;
	}
};

} // end namespace ez
