#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "lib/stringinfo.h"
#include "hstore.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

char *hstore_lookup(HStore *hs, char *key, int keylen, int *vallenp);
void output_append(StringInfoData *output, char *val, int vallen, char type);

Datum replace(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(replace);

Datum replace(PG_FUNCTION_ARGS) {
  // get user input text
  text *format_string_text = PG_GETARG_TEXT_PP(0);
  char *start_ptr;
  char *end_ptr;
  char *cp;
  char *key_ptr;
  int length; // use int for length to accomodate hstoreFindKey()

  // upon scan, output stores the text before and after the format specifier
  StringInfoData output;
  int state = 0;

  HStore *hs = (HStore *) PG_GETARG_DATUM(1);

  // result is used to store the returned result which is output converted to text
  text *result;

  start_ptr = VARDATA_ANY(format_string_text);
  end_ptr = start_ptr + VARSIZE_ANY_EXHDR(format_string_text);
  initStringInfo(&output);

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
      key_ptr = cp + 1;
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
      int vallen;
      char *val = hstore_lookup(hs, key_ptr, length, &vallen);
      if (val != NULL)
        output_append(&output, val, vallen, *cp);
      state = 0;
    }
  }

  result = cstring_to_text_with_len(output.data, output.len);

  pfree(output.data);

  PG_RETURN_TEXT_P(result);
}

char *hstore_lookup(HStore *hs, char *key, int keylen, int *vallenp) {
  // define a pointer to a function which receives a pointer to an HStore, an int pointer, a char pointer, and an int and returns an int
  int (*hstoreFindKey)(HStore*, int*, char*, int) = (int (*)(HStore*, int*, char*, int)) load_external_function("hstore", "hstoreFindKey", true, NULL);

  int idx = hstoreFindKey(hs, NULL, key, keylen);

  if (idx < 0) {
    elog(WARNING, "Invalid key\n");
    return NULL;
  }
  if (HSTORE_VALISNULL(ARRPTR(hs), idx)) {
    elog(WARNING, "Null value\n");
    return NULL;
  }

  *vallenp = HSTORE_VALLEN(ARRPTR(hs), idx);
  return HSTORE_VAL(ARRPTR(hs), STRPTR(hs), idx);
}

void output_append(StringInfoData *output, char *val, int vallen, char type) {
  char *string = val;
  int length = vallen;

  if (type == 'I') {
    string = (char *) quote_identifier(val);
    length = strlen(string);
  }
  else if (type == 'L') {
    string = (char *) quote_literal_cstr(val);
    length = strlen(string);
  }
  appendBinaryStringInfo(output, string, length);

  if (type == 'L')
    pfree(string);
}
