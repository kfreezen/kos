#include <common.h>

#define FL_UNSIGNED   1
#define FL_NEG        2
#define FL_OVERFLOW   4
#define FL_READDIGIT  8

#define LONG_MIN 0x80000000L
#define LONG_MAX ~LONG_MIN

#define ULONG_MAX 0xFFFFFFFFUL

#define KERNEL

int isdigit(int c) {
	return (c>=0x30 && c<=0x39) ? 1 : 0;
}

int islower(int c) {
	return (c>='a' && c<='z') ? 1 : 0;
}

int isupper(int c) {
	return (c>='A' && c<='Z') ? 1 : 0;
}

int isalpha(int c) {
	return (islower(c)||isupper(c)) ? 1 : 0;
}

int isspace(int c) {
	return (c==' ' || c=='\t' || c=='\n') ? 1 : 0;
}

int toupper(int c) {
	if(islower(c)) {
		return c + ('A' - 'a');
	} else {
		return c;
	}
}

static unsigned long strtoxl(const char *nptr, char **endptr, int ibase, int flags) {
  const unsigned char *p;
  char c;
  unsigned long number;
  unsigned digval;
  unsigned long maxval;

  p = (const unsigned char *) nptr;
  number = 0;

  c = *p++;
  while (isspace(c)) c = *p++;

  if (c == '-') {
    flags |= FL_NEG;
    c = *p++;
  } else if (c == '+') {
    c = *p++;
  }

  if (ibase < 0 || ibase == 1 || ibase > 36) {
    if (endptr) *endptr = (char *) nptr;
    return 0L;
  } else if (ibase == 0) {
    if (c != '0') {
      ibase = 10;
    } else if (*p == 'x' || *p == 'X') {
      ibase = 16;
    } else {
      ibase = 8;
    }
  }

  if (ibase == 16) {
    if (c == '0' && (*p == 'x' || *p == 'X')) {
      ++p;
      c = *p++;
    }
  }

  maxval = ULONG_MAX / ibase;

  for (;;) {
    if (isdigit(c)) {
      digval = c - '0';
    } else if (isalpha(c)) {
      digval = toupper(c) - 'A' + 10;
    } else {
      break;
    }

    if (digval >= (unsigned) ibase) break;

    flags |= FL_READDIGIT;

    if (number < maxval || (number == maxval && (unsigned long) digval <= ULONG_MAX % ibase)) {
      number = number * ibase + digval;
    } else {
      flags |= FL_OVERFLOW;
    }

    c = *p++;
  }

  --p;

  if (!(flags & FL_READDIGIT)) {
    if (endptr) p = nptr;
    number = 0;
  } else if ((flags & FL_OVERFLOW) || (!(flags & FL_UNSIGNED) && (((flags & FL_NEG) && (number < LONG_MIN)) || (!(flags & FL_NEG) && (number > LONG_MAX))))) {
#ifndef KERNEL
    errno = ERANGE;
#endif

    if (flags & FL_UNSIGNED) {
      number = ULONG_MAX;
    } else if (flags & FL_NEG) {
      number = LONG_MIN;
    } else {
      number = LONG_MAX;
    }
  }

  if (endptr) *endptr = (char *) p;

  if (flags & FL_NEG) number = (unsigned long) (-(long) number);

  return number;
}

long strtoul(const char *nptr, char **endptr, int ibase) {
  return (long) strtoxl(nptr, endptr, ibase, FL_UNSIGNED);
}
