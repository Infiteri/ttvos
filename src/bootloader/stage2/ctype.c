#include "ctype.h"

bool IsLower(char chr) { return chr >= 'a' && chr <= 'z'; }

char ToUpper(char chr) { return IsLower(chr) ? (chr - 'a' + 'A') : chr; }
