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
    if (state == 0 && *cp != '{') {
      appendStringInfoCharMacro(&output, *cp);
      state = 0;
    }
    else if (state == 0 && *cp == '{') {
      key_start_ptr = cp + 1;
      state = 1;
    }
    else if (state == 1 && *cp != '}') {
      key_end_ptr = cp;
      state = 1;
    }
    else if (state == 1 && *cp == '}') {
      int validx = hstoreFindKey(hs, NULL, key_start_ptr, (key_end_ptr - key_start_ptr) + 1);
      if (validx >= 0 && !HSTORE_VALISNULL(entries, validx)) {
        appendBinaryStringInfo(&output, HSTORE_VAL(entries, STRPTR(hs), validx), HSTORE_VALLEN(entries, validx));
      }
      state = 0;
      continue;
    }
  }

  result = cstring_to_text_with_len(output.data, output.len);

  pfree(output.data);

  PG_RETURN_TEXT_P(result);
}
