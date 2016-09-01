#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "lib/stringinfo.h"
#include "hstore.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

Datum replace(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(replace);

Datum replace(PG_FUNCTION_ARGS) {
  // get user input text
  text *format_string_text = PG_GETARG_TEXT_PP(0);
  char *start_ptr;
  char *end_ptr;
  char *cp;
  char *key_start_ptr;
  char *key_end_ptr;

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
      state = 2;
    }
    else if (state == 2 && *cp != '(' && *cp != ')') {
      state = 2;
    }
    else if (state == 2 && *cp == ')') {
      key_end_ptr = cp - 1;
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
    else if (state == 3 && *cp == 's') {
      int validx = hstoreFindKey(hs, NULL, key_start_ptr, (key_end_ptr - key_start_ptr) + 1);
      if (validx >= 0 && !HSTORE_VALISNULL(entries, validx)) {
        appendBinaryStringInfo(&output, HSTORE_VAL(entries, STRPTR(hs), validx), HSTORE_VALLEN(entries, validx));
      }
      else {
        StringInfoData testkey;
        initStringInfo(&testkey);
        appendBinaryStringInfo(&testkey, HSTORE_VAL(entries, STRPTR(hs), validx), HSTORE_VALLEN(entries, validx));
        elog(WARNING, "Invalid key: %s\n", testkey.data);
        elog(WARNING, "Start char: %c End char: %c\n", *key_start_ptr, *key_end_ptr);
      }
      state = 0;
      continue;
    }
    else if (state == 3 && *cp == 'I') {
      int validx = hstoreFindKey(hs, NULL, key_start_ptr, (key_end_ptr - key_start_ptr) + 1);
      if (validx >= 0 && !HSTORE_VALISNULL(entries, validx)) {
        const char *istring = quote_identifier(HSTORE_VAL(entries, STRPTR(hs), validx));
        int ilength = strlen(istring);
        appendBinaryStringInfo(&output, istring, ilength);
      }
      else {
        elog(WARNING, "Start char: %c End char: %c\n", *key_start_ptr, *key_end_ptr);
      }
      state = 0;
      continue;
    }
    else if (state == 3 && *cp == 'L') {
      int validx = hstoreFindKey(hs, NULL, key_start_ptr, (key_end_ptr - key_start_ptr) + 1);
      if (validx >= 0 && !HSTORE_VALISNULL(entries, validx)) {
        char *lstring = quote_literal_cstr(HSTORE_VAL(entries, STRPTR(hs), validx));
        int llength = strlen(lstring);
        appendBinaryStringInfo(&output, lstring, llength);
        pfree(lstring);
      }
      else {
        elog(WARNING, "Start char: %c End char: %c\n", *key_start_ptr, *key_end_ptr);
      }
      state = 0;
      continue;
    }
    else if (state == 4 && *cp >= '0' && *cp <= '9') {
      // TO-DO: modify mwidth
      state = 4;
    }
    else if (state == 4 && *cp == 's') {
      int validx = hstoreFindKey(hs, NULL, key_start_ptr, (key_end_ptr - key_start_ptr) + 1);
      if (validx >= 0 && !HSTORE_VALISNULL(entries, validx)) {
        appendBinaryStringInfo(&output, HSTORE_VAL(entries, STRPTR(hs), validx), HSTORE_VALLEN(entries, validx));
      }
      else {
        StringInfoData testkey;
        initStringInfo(&testkey);
        appendBinaryStringInfo(&testkey, HSTORE_VAL(entries, STRPTR(hs), validx), HSTORE_VALLEN(entries, validx));
        elog(WARNING, "Invalid key: %s\n", testkey.data);
        elog(WARNING, "Start char: %c End char: %c\n", *key_start_ptr, *key_end_ptr);
      }
      state = 0;
      continue;
    }
    else if (state == 4 && *cp == 'I') {
      int validx = hstoreFindKey(hs, NULL, key_start_ptr, (key_end_ptr - key_start_ptr) + 1);
      if (validx >= 0 && !HSTORE_VALISNULL(entries, validx)) {
        const char *istring = quote_identifier(HSTORE_VAL(entries, STRPTR(hs), validx));
        int ilength = strlen(istring);
        appendBinaryStringInfo(&output, istring, ilength);
      }
      else {
        elog(WARNING, "Start char: %c End char: %c\n", *key_start_ptr, *key_end_ptr);
      }
      state = 0;
      continue;
    }
    else if (state == 4 && *cp == 'L') {
      int validx = hstoreFindKey(hs, NULL, key_start_ptr, (key_end_ptr - key_start_ptr) + 1);
      if (validx >= 0 && !HSTORE_VALISNULL(entries, validx)) {
        char *lstring = quote_literal_cstr(HSTORE_VAL(entries, STRPTR(hs), validx));
        int llength = strlen(lstring);
        appendBinaryStringInfo(&output, lstring, llength);
        pfree(lstring);
      }
      else {
        elog(WARNING, "Start char: %c End char: %c\n", *key_start_ptr, *key_end_ptr);
      }
      state = 0;
      continue;
    }
  }

  result = cstring_to_text_with_len(output.data, output.len);

  pfree(output.data);

  PG_RETURN_TEXT_P(result);
}
