/*
This file is part of ezOptionParser. See MIT-LICENSE.

Copyright (C) 2011,2012,2014 Remik Ziemlinski <first d0t surname att gmail>

CHANGELOG

v0.0.0 20110505 rsz Created.
v0.1.0 20111006 rsz Added validator.
v0.1.1 20111012 rsz Fixed validation of ulonglong.
v0.1.2 20111126 rsz Allow flag names start with alphanumeric (previously, flag had to start with alpha).
v0.1.3 20120108 rsz Created work-around for unique id generation with IDGenerator that avoids retarded c++ translation unit linker errors with single-header static variables. Forced inline on all methods to please retard compiler and avoid multiple def errors.
v0.1.4 20120629 Enforced MIT license on all files.
v0.2.0 20121120 Added parseIndex to OptionGroup.
v0.2.1 20130506 Allow disabling doublespace of OPTIONS usage descriptions.
v0.2.2 20140504 Jose Santiago added compiler warning fixes.
                Bruce Shankle added a crash fix in description printing.
*/
#ifndef EZ_OPTION_PARSER_H
#define EZ_OPTION_PARSER_H

#include "al2o3_platform/platform.h"
#include "al2o3_tinystl/vector.hpp"
#include "al2o3_tinystl/unordered_map.hpp"
#include "al2o3_tinystl/string.hpp"
#include "al2o3_vfile/vfile.h"

namespace ez {

/* Validate a value by checking:
- if as string, see if converted value is within datatype's limits,
- and see if falls within a desired range,
- or see if within set of given list of values.

If comparing with a range, the values list must contain one or two values. One value is required when comparing with <, <=, >, >=. Use two values when requiring a test such as <x<, <=x<, <x<=, <=x<=.
A regcomp/regexec based class could be created in the future if a need arises.
*/
class OptionValidator {
public:
	OptionValidator(const char *_type, const char *_op = 0, const char *list = 0, bool _insensitive = false);
	OptionValidator(char _type);

	OptionValidator(char _type, char _op, const int8_t *list, int _size);
	OptionValidator(char _type, char _op, const uint8_t *list, int _size);
	OptionValidator(char _type, char _op, const int16_t *list, int _size);
	OptionValidator(char _type, char _op, const uint16_t *list, int _size);
	OptionValidator(char _type, char _op, const int32_t *list, int _size);
	OptionValidator(char _type, char _op, const uint32_t *list, int _size);
	OptionValidator(char _type, char _op, const int64_t *list, int _size = 0);
	OptionValidator(char _type, char _op, const uint64_t *list, int _size);
	OptionValidator(char _type, char _op, const float *list, int _size);
	OptionValidator(char _type, char _op, const double *list, int _size);
	OptionValidator(char _type, char _op, const char **list, int _size, bool _insensitive);
	~OptionValidator();

	bool isValid(const tinystl::string *value);
	void print();
	void reset();

	/* If value must be in custom range, use these comparison modes. */
	enum OP {
		NOOP = 0,
		LT, /* value < list[0] */
		LE, /* value <= list[0] */
		GT, /* value > list[0] */
		GE, /* value >= list[0] */
		GTLT, /* list[0] < value < list[1] */
		GELT, /* list[0] <= value < list[1] */
		GELE, /* list[0] <= value <= list[1] */
		GTLE, /* list[0] < value <= list[1] */
		IN /* if value is in list */
	};

	enum TYPE { NOTYPE = 0, S1, U1, S2, U2, S4, U4, S8, U8, F, D, T };
	enum TYPE2 { NOTYPE2 = 0, INT8, UINT8, INT16, UINT16, INT32, UINT32, INT64, UINT64, FLOAT, DOUBLE, TEXT };

	union {
		uint8_t *u1;
		int8_t *s1;
		uint16_t *u2;
		int16_t *s2;
		uint32_t *u4;
		int32_t *s4;
		uint64_t *u8;
		int64_t *s8;
		float *f;
		double *d;
		tinystl::string **t;
	};

	char op;
	bool quiet;
	short id;
	char type;
	int size;
	bool insensitive;
};

class OptionGroup {
public:
	OptionGroup() : delim(0), expectArgs(0), isRequired(false), isSet(false) {}

	~OptionGroup() {
		int i;
		for (i = 0; i < (long int) flags.size(); ++i)
			delete flags[i];

		flags.clear();
		parseIndex.clear();
		clearArgs();
	};

	void clearArgs();
	void getFloat(float &);
	void getDouble(double &);
	void getFloats(tinystl::vector<float> &);
	void getDoubles(tinystl::vector<double> &);
	void getMultiFloats(tinystl::vector<tinystl::vector<float> > &);
	void getMultiDoubles(tinystl::vector<tinystl::vector<double> > &);

	void getString(tinystl::string &);
	void getStrings(tinystl::vector<tinystl::string> &);
	void getMultiStrings(tinystl::vector<tinystl::vector<tinystl::string> > &);

	void getInt64(int64_t &);
	void getInt64s(tinystl::vector<int64_t> &);
	void getMultiInt64s(tinystl::vector<tinystl::vector<int64_t> > &);

	// defaults value regardless of being set by user.
	tinystl::string defaults;
	// If expects arguments, this will delimit arg list.
	char delim;
	// If not 0, then number of delimited args. -1 for arbitrary number.
	int expectArgs;
	// Descriptive help message shown in usage instructions for option.
	tinystl::string help;
	// 0 or 1.
	bool isRequired;
	// A list of flags that denote this option, i.e. -d, --dimension.
	tinystl::vector<tinystl::string *> flags;
	// If was set (or found).
	bool isSet;
	// Lists of arguments, per flag instance, after splitting by delimiter.
	tinystl::vector<tinystl::vector<tinystl::string *> *> args;
	// Index where each group was parsed from input stream to track order.
	tinystl::vector<int> parseIndex;
};

typedef tinystl::unordered_map<int, OptionValidator *> ValidatorMap;

class OptionParser {
public:
	// How to layout usage descriptions with the option flags.
	enum Layout { ALIGN, INTERLEAVE, STAGGER };

	~OptionParser();

	void add(const char *defaults,
					 bool required,
					 int expectArgs,
					 char delim,
					 const char *help,
					 const char *flag1,
					 OptionValidator *validator = 0);
	void add(const char *defaults,
					 bool required,
					 int expectArgs,
					 char delim,
					 const char *help,
					 const char *flag1,
					 const char *flag2,
					 OptionValidator *validator = 0);
	void add(const char *defaults,
					 bool required,
					 int expectArgs,
					 char delim,
					 const char *help,
					 const char *flag1,
					 const char *flag2,
					 const char *flag3,
					 OptionValidator *validator = 0);
	void add(const char *defaults,
					 bool required,
					 int expectArgs,
					 char delim,
					 const char *help,
					 const char *flag1,
					 const char *flag2,
					 const char *flag3,
					 const char *flag4,
					 OptionValidator *validator = 0);
	OptionGroup *get(const char *name);
	void getUsage(tinystl::string &usage, int width = 80, Layout layout = ALIGN);
	void getUsageDescriptions(tinystl::string &usage, int width = 80, Layout layout = STAGGER);
	bool gotExpected(tinystl::vector<tinystl::string> &badOptions);
	bool gotRequired(tinystl::vector<tinystl::string> &badOptions);
	bool gotValid(tinystl::vector<tinystl::string> &badOptions, tinystl::vector<tinystl::string> &badArgs);

	bool exportFile(VFile_Handle handle, bool all = false);
	bool importFile(VFile_Handle handle, char comment = '#');

	int isSet(const char *name);
	int isSet(tinystl::string &name);
	void parse(int argc, const char *argv[]);
	void prettyPrint(tinystl::string &out);
	void reset();
	void resetArgs();

	// Insert extra empty line betwee each option's usage description.
	char doublespace;
	// General description in human language on what the user's tool does.
	// It's the first section to get printed in the full usage message.
	tinystl::string overview;
	// A synopsis of command and options usage to show expected order of input arguments.
	// It's the second section to get printed in the full usage message.
	tinystl::string syntax;
	// Example (third) section in usage message.
	tinystl::string example;
	// Final section printed in usage message. For contact, copyrights, version info.
	tinystl::string footer;
	// Map from an option to an Id of its parent group.
	tinystl::unordered_map<tinystl::string, int> optionGroupIds;
	// Unordered collection of the option groups.
	tinystl::vector<OptionGroup *> groups;
	// Store unexpected args in input.
	tinystl::vector<tinystl::string *> unknownArgs;
	// List of args that occur left-most before first option flag.
	tinystl::vector<tinystl::string *> firstArgs;
	// List of args that occur after last right-most option flag and its args.
	tinystl::vector<tinystl::string *> lastArgs;
	// List of validators.
	ValidatorMap validators;
	// Maps group id to a validator index into vector of validators. Validator index is -1 if there is no validator for group.
	tinystl::unordered_map<int, int> groupValidators;
};

} // end namespace
#endif
