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
  // get user input text
  text *format_string_text = PG_GETARG_TEXT_PP(0);
  const char *start_ptr;
  const char *end_ptr;
  const char *cp;

  // upon scan, key stores the text within the format specifier
  // must be a StringInfoData so appendStringInfoCharMacro can be used
  StringInfoData key;
  // upon scan, output stores the text before and after the format specifier
  StringInfoData output;
  // length will be the length of the output string before the initial {
  int length = 0;
  int state = 0;

  // tkey will store the text version of the StringInfoData key parsed from the format_string_text
  text *tkey;

  // get access to the function hstore_fetchval
  PGFunction hstore_fetchval = load_external_function(
      "hstore", "hstore_fetchval", true, NULL);
  // get user input hstore
  Datum hstore = PG_GETARG_DATUM(1);

  // value will store the result of calling hstore_fetchval with the key
  Datum value;  
  // strval will store the converted cstring version of the value
  char *strval;

  // formatoutput is the StringInfoData which is used to copy in the text and the value retrieved
  StringInfoData formatoutput;

  // result is used to store the returned result which is formatoutput converted to text
  text *result;

  start_ptr = VARDATA_ANY(format_string_text);
  end_ptr = start_ptr + VARSIZE_ANY_EXHDR(format_string_text);
  initStringInfo(&output);
  initStringInfo(&key);

  for (cp = start_ptr; cp < end_ptr; cp++) {

    if (state == 0 && *cp != '{') {
      appendStringInfoCharMacro(&output, *cp);
      state = 0;
    }
    else if (state == 0 && *cp == '{') {
      length = output.len;
      state = 1;
    }
    else if (state == 1 && *cp != '}') {
      appendStringInfoCharMacro(&key, *cp);
      state = 1;
    }
    else if (state == 1 && *cp == '}') {
      state = 0;
      // TODO: make it take multiple keys
      /* keycounter++; */
    }
  }

  /* // TestStart: Check if key is correct value */
  /* text *keytest; */
  /* if (strcmp(key.data, "food") != 0) { */
  /*   char *keytestreject = "Key is not correct value"; */
  /*   keytest = cstring_to_text(keytestreject); */
  /*   PG_RETURN_TEXT_P(keytest); */
  /* } */
  /* else { */
  /*   keytest = cstring_to_text_with_len(key.data, key.len); */
  /*   PG_RETURN_TEXT_P(keytest); */
  /* } */
  /* // TestEnd */

  tkey = cstring_to_text_with_len(key.data, key.len);
  
  value = DirectFunctionCall2(
    hstore_fetchval, hstore, (Datum) tkey);
  if (value == (Datum) 0) {
    PG_RETURN_NULL();
  }

  /* // TestStart: Return value returned */
  /* PG_RETURN_TEXT_P(value); */
  /* // TestEnd */

  strval = text_to_cstring((text*) value);
  int lenval = strlen(strval);

  /* // TestStart: Check if the value retrieved and converted  matches that in the hstore */
  /* text *valtest; */
  /* if (strcmp(strval, "pork") != 0) { */
  /*   char *valtestreject = "Value is not correct"; */
  /*   valtest = cstring_to_text(valtestreject); */
  /*   PG_RETURN_TEXT_P(valtest); */
  /* } */
  /* else { */
  /*   valtest = cstring_to_text_with_len(strval, lenval); */
  /*   PG_RETURN_TEXT_P(valtest); */
  /* } */
  /* // TestEnd */

  initStringInfo(&formatoutput);

  enlargeStringInfo(&formatoutput, lenval + output.len);

  memcpy(formatoutput.data, output.data, length);
  formatoutput.len += length;
  formatoutput.data[formatoutput.len] = '\0';

  /* // TestStart: Check current value of formatoutput */
  /* text* check = cstring_to_text_with_len(formatoutput.data, formatoutput.len); */
  /* PG_RETURN_TEXT_P(check); */
  /* // TestEnd */

  memcpy(formatoutput.data + formatoutput.len, strval, lenval);
  formatoutput.len += lenval;
  formatoutput.data[formatoutput.len] = '\0';

  /* // TestStart: Check current value of formatoutput */
  /* text* check = cstring_to_text_with_len(formatoutput.data, formatoutput.len); */
  /* PG_RETURN_TEXT_P(check); */
  /* // TestEnd */

  memcpy(formatoutput.data + formatoutput.len, output.data + length, output.len - length);
  formatoutput.len += output.len - length;
  formatoutput.data[formatoutput.len] = '\0';

  result = cstring_to_text_with_len(formatoutput.data, formatoutput.len);

  pfree(key.data);
  pfree(output.data);
  pfree(formatoutput.data);

  PG_RETURN_TEXT_P(result);
}
