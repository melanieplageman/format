#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "lib/stringinfo.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

Datum replace(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(replace);

Datum replace(PG_FUNCTION_ARGS) {
  PGFunction hstore_fetchval = load_external_function(
      "hstore", "hstore_fetchval", true, NULL);
  text *format_string_text = PG_GETARG_TEXT_PP(0);
  char *string = text_to_cstring(format_string_text);
  Datum hstore = PG_GETARG_DATUM(1);
  Datum value;
  Datum dresult;

  StringInfoData key;
  initStringInfo(&key);

  StringInfoData output;
  initStringInfo(&output);
  char *cursor = output.data;
  char *placeholder = NULL;
  int length = 0;

  StringInfoData formatoutput;
  initStringInfo(&formatoutput);

  int state = 0;
  for (size_t i = 0; string[i] != '\0'; i++) {
    if (state == 0 && string[i] != '{') {
      appendStringInfoCharMacro(&output, string[i]);
      cursor++;
      state = 0;
    }
    else if (state == 0 && string[i] == '{') {
      placeholder = cursor;
      length = output.len;
      state = 1;
    }
    else if (state == 1 && string[i] != '}') {
      appendStringInfoCharMacro(&key, string[i]);
      state = 1;
    }
    else if (state == 1 && string[i] == '}') {
      state = 0;
    }
  }

  text *tkey = cstring_to_text_with_len(key.data, key.len);
  pfree(key.data);
  
  value = DirectFunctionCall2(
    hstore_fetchval, hstore, (Datum) tkey);
  if (value == (Datum) 0)
    PG_RETURN_NULL();

  strncpy(formatoutput.data, output.data, length);
  formatoutput.data[length] = '\0';
  strcat(formatoutput.data, text_to_cstring(value));
  strcat(formatoutput.data, output.data+length);

  text *result = cstring_to_text_with_len(formatoutput.data, formatoutput.len);
  dresult = (Datum) result; 

  pfree(output.data);
  pfree(formatoutput.data);

  PG_RETURN_TEXT_P(dresult);
}
