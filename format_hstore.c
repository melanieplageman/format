#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "lib/stringinfo.h"
#include "hstore.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

typedef HStore *(*hstoreUpgradeF)(Datum);

char *hstore_lookup(HStore *hs, char *key, int keylen, int *vallenp);
void output_append(StringInfoData *output, char *val, int vallen, char type, int width, bool align_to_left);
char *option_format(StringInfoData *output, char *string, int length, int width, bool align_to_left);

// Returns a formatted string when provided with named arguments
Datum format_hstore(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(format_hstore);

// format_hstore() is STRICT so it returns NULL when any arguments are NULL
Datum format_hstore(PG_FUNCTION_ARGS) {
  text *format_string_text = PG_GETARG_TEXT_PP(0);

  // Manually do PG_GETARG_HS() as in hstore.h
  hstoreUpgradeF hstoreUpgrade = (hstoreUpgradeF) load_external_function(
      "hstore", "hstoreUpgrade", true, NULL);
  HStore *hs = hstoreUpgrade(PG_GETARG_DATUM(1));

  char *start_ptr;
  char *end_ptr;
  char *cp;
  char *key_ptr;
  int length; // use int for length to accomodate hstoreFindKey(); also a varlena size is guaranteed not to overflow int
  int width;
  bool align_to_left = false; // used for conversion flag

  StringInfoData output; // output is the running string to which text is being appended during the scanning and parsing
  int state = 0;

  start_ptr = VARDATA_ANY(format_string_text);
  end_ptr = start_ptr + VARSIZE_ANY_EXHDR(format_string_text);
  initStringInfo(&output);

  // Scan format string looking for format specifiers, conversion flags, and specified width
  for (cp = start_ptr; cp < end_ptr; cp++) {
    // If text is not the start of a format specifier, append it to output
    if (state == 0 && *cp != '%') {
      appendStringInfoCharMacro(&output, *cp);
      state = 0;
    }
    else if (state == 0 && *cp == '%') {
      width = 0;
      align_to_left = false;
      state = 1;
    }
    // If two adjacent format start specifiers, '%', are found, consider it an escaped '%' character and append as usual
    else if (state == 1 && *cp == '%') {
      appendStringInfoCharMacro(&output, *cp);
      state = 0;
    }
    // If a single format start specifier is followed by a '(' character, set key_ptr and initialize length
    else if (state == 1 && *cp == '(') {
      key_ptr = cp + 1;
      length = 0;
      state = 2;
    }
    // If a single format start specifier is followed by any character other than a '%' or a '(', this is an error
    else if (state == 1 && *cp != '(' && *cp != '%') {
      elog(ERROR, "Unsupported format character %c\n", *cp);
    }
    // If a '%' is followed by a '(' follwed by another '(', this is an error
    else if (state == 2 && *cp == '(') {
      elog(ERROR, "Incomplete format key");
    }
    // Format specifier key text
    else if (state == 2 && *cp != '(' && *cp != ')') {
      length += 1;
      state = 2;
    }
    else if (state == 2 && *cp == ')') {
      state = 3;
    }
    // A ')' character must be followed by at least a format type (either 's', 'I', or 'L')
    // and can optionally be followed by a conversion flag ('-') and a width before the format type character
    // These must appear in the order 1) conversion flag 2) width 3) format type
    else if (state == 3) {
      if (*cp == '-') {
        align_to_left = true;
        state = 3;
      }
      else if (*cp >= '1' && *cp <= '9') {
        width = *cp - '0';
        state = 4;
      }
      // Once the format type character is found, the format specifier is complete and the key is available for
      // lookup in the provided hstore. Once the value is found, it is appended to output in the usual way
      else if (*cp == 's' || *cp == 'I' || *cp == 'L') {
        int vallen;
        char *val = hstore_lookup(hs, key_ptr, length, &vallen);
        if (val != NULL)
          output_append(&output, val, vallen, *cp, width, align_to_left);
        state = 0;
      }
      // If characters other than a conversion flag, format type, or width are found, this is an error
      else {
        elog(ERROR, "Unsupported format character %c\n", *cp);
      }
    }
    // Cover the case in which the width is more than one digit
    else if (state == 4) {
      if (*cp >= '0' && *cp <= '9') {
        width = width * 10 + (*cp - '0');
        state = 4;
      }
      else if (*cp == 's' || *cp == 'I' || *cp == 'L') {
        int vallen;
        char *val = hstore_lookup(hs, key_ptr, length, &vallen);
        if (val != NULL)
          output_append(&output, val, vallen, *cp, width, align_to_left);
        state = 0;
      }
      else {
        elog(ERROR, "Unsupported format character %c\n", *cp);
      }
    }
  }
  // State must be 0 by the end of the format string. A non-zero state at the end of a format string indicates a malformed string
  // This could be due to lack of required specifiers, such as format type
  if (state != 0) {
    elog(ERROR, "Invalid format string\n");
  }

  text *result = cstring_to_text_with_len(output.data, output.len);

  pfree(output.data);

  PG_RETURN_TEXT_P(result);
}

char *hstore_lookup(HStore *hs, char *key, int keylen, int *vallenp) {
  // Define a pointer to a function which receives a pointer to an HStore, an int pointer, a char pointer, and an int and returns an int
  int (*hstoreFindKey)(HStore*, int*, char*, int) = (int (*)(HStore*, int*, char*, int)) load_external_function("hstore", "hstoreFindKey", true, NULL);

  int idx = hstoreFindKey(hs, NULL, key, keylen);

  // If a key is not found, scanning will continue
  if (idx < 0) {
    elog(WARNING, "Key not found\n");
    return NULL;
  }
  // Issue a warning in the case of a NULL value. This means that all valid values and format text input will be output.
  if (HSTORE_VALISNULL(ARRPTR(hs), idx)) {
    elog(WARNING, "Null value\n");
    return NULL;
  }

  *vallenp = HSTORE_VALLEN(ARRPTR(hs), idx);
  return HSTORE_VAL(ARRPTR(hs), STRPTR(hs), idx);
}

char *option_format(StringInfoData *output, char *string, int length, int width, bool align_to_left) {
  // Parse the optional portions of the format specifier
  if (width == 0) {
    appendBinaryStringInfo(output, string, length);
    return string;
  }

  if (align_to_left) {
    // left justify
    appendBinaryStringInfo(output, string, length);
    if (length < width) {
      appendStringInfoSpaces(output, width - length);
    }
  }
  else {
    // right justify
    if (length < width) {
      appendStringInfoSpaces(output, width - length);
    }
    appendBinaryStringInfo(output, string, length);
  }
  return string;
}

void output_append(StringInfoData *output, char *val, int vallen, char type, int width, bool align_to_left) {
  // Need to allocate memory for a new, null-terminated string
  // The return value from hstore_lookup() is not necessarily null-terminated
  char *string = palloc(vallen * sizeof(char) + 1);
  memcpy(string, val, vallen);
  string[vallen] = '\0';
  int length = vallen;

  if (type == 'I') {
    // quote_identifier() sometimes returns a palloc'd string and sometimes returns the original string
    string = (char *) quote_identifier(string);
    length = strlen(string);
  }
  else if (type == 'L') {
    string = (char *) quote_literal_cstr(string);
    length = strlen(string);
  }

  string = option_format(output, string, length, width, align_to_left);
  if (type == 'L') {
    pfree(string);
  }

}
