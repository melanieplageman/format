#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "lib/stringinfo.h"
#include "hstore.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

void output_append(StringInfoData *output, char *val, int vallen, char type);

Datum replace(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(replace);

Datum replace(PG_FUNCTION_ARGS) {
  // get user input text
  text *format_string_text = PG_GETARG_TEXT_PP(0);
  char *start_ptr;
  char *end_ptr;
  char *cp;
  char *key_start_ptr;
  int length; // use int for length to accomodate hstoreFindKey()

  // upon scan, output stores the text before and after the format specifier
  StringInfoData output;
  int state = 0;

  // define a pointer to a function which receives a pointer to an HStore, an int pointer, a char pointer, and an int and returns an int
  int (*hstoreFindKey)(HStore*, int*, char*, int) = (int (*)(HStore*, int*, char*, int)) load_external_function("hstore", "hstoreFindKey", true, NULL);

  HStore *hs = (HStore *) PG_GETARG_DATUM(1);

  // result is used to store the returned result which is output converted to text
  text *result;

  start_ptr = VARDATA_ANY(format_string_text);
  end_ptr = start_ptr + VARSIZE_ANY_EXHDR(format_string_text);
  initStringInfo(&output);

  HEntry *entries = ARRPTR(hs);

  for (cp = start_ptr; cp < end_ptr; cp++) {
    if (state == 0 && *cp != '%') {
      appendStringInfoCharMacro(&output, *cp);
      state = 0;
    }
    else if (state == 0 && *cp == '%') {
      state = 1;
    }
    else if (state == 1 && *cp == '%') {
      appendStringInfoCharMacro(&output, *cp);
      state = 0; 
    }
    else if (state == 1 && *cp == '(') {
      key_start_ptr = cp + 1;
      length = 0;
      state = 2;
    }
    else if (state == 2 && *cp != '(' && *cp != ')') {
      length += 1;
      state = 2;
    }
    else if (state == 2 && *cp == ')') {
      /* key_end_ptr = cp - 1; */
      state = 3;
    }
    else if (state == 3 && *cp == '-') {
      // TO-DO: set conversion flag
      state = 3;
    }
    else if (state == 3 && *cp >= '1' && *cp <= '9') {
      // TO-DO: set width
      state = 4;
    }
    else if (state == 4 && *cp >= '0' && *cp <= '9') {
      // TO-DO: modify mwidth
      state = 4;
    }
    else if ((state == 3 || state == 4) && (*cp == 's' || *cp == 'I' || *cp == 'L')) {
      int validx = hstoreFindKey(hs, NULL, key_start_ptr, length);
      if (validx >= 0 && !HSTORE_VALISNULL(entries, validx)) {
        output_append(&output, HSTORE_VAL(entries, STRPTR(hs), validx), HSTORE_VALLEN(entries, validx), 'I');
      }
      else {
        StringInfoData testkey;
        initStringInfo(&testkey);
        appendBinaryStringInfo(&testkey, HSTORE_VAL(entries, STRPTR(hs), validx), HSTORE_VALLEN(entries, validx));
        elog(WARNING, "Invalid key: %s\n", testkey.data);
      }
      state = 0;
    }
  }

  result = cstring_to_text_with_len(output.data, output.len);

  pfree(output.data);

  PG_RETURN_TEXT_P(result);
}

void output_append(StringInfoData *output, char *val, int vallen, char type) {
  const char *string = val;
  int length = vallen;

  if (type == 'I') {
    string = quote_identifier(val);
    length = strlen(string);
  }
  else if (type == 'L') {
    string = quote_literal_cstr(val);
    length = strlen(string);
  }
  appendBinaryStringInfo(output, string, length);

  if (type == 'L')
    pfree(string);
}
