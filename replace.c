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
  const char *tracker;

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
  initStringInfo(&formatoutput);

  // result is used to store the returned result which is formatoutput converted to text
  text *result;

  start_ptr = VARDATA_ANY(format_string_text);
  end_ptr = start_ptr + VARSIZE_ANY_EXHDR(format_string_text);
  initStringInfo(&output);
  initStringInfo(&key);
  
  // need to leave inner loop each time an entire key is discovered and look it up
  tracker = start_ptr;
  cp = tracker;
  while (cp < end_ptr) {

    for (cp = tracker; cp < end_ptr; cp++) {

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
        cp++;
        break;
      }
    }

    if (cp == end_ptr) break;

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
    
    // look up key in hstore and retrieve value
    value = DirectFunctionCall2(
      hstore_fetchval, hstore, (Datum) tkey);
    /* if (value == (Datum) 0) { */
    /*   PG_RETURN_NULL(); */
    /* } */

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

    // enlarge formatoutput to fit the length of the output until the value and the value
    enlargeStringInfo(&formatoutput, lenval + length);

    // copy the first part of the input string to the format string
    memcpy((&formatoutput)->data + (&formatoutput)->len, (&output)->data, length);

    // increment formatoutput length by the amount of the first part of the input string
    (&formatoutput)->len += length;

    (&formatoutput)->data[(&formatoutput)->len] = '\0';

    /* // TestStart: Check current value of formatoutput */
    /* text* check = cstring_to_text_with_len(formatoutput.data, formatoutput.len); */
    /* PG_RETURN_TEXT_P(check); */
    /* // TestEnd */

    // copy the value retrieved to the end of the formatoutput string
    memcpy((&formatoutput)->data + (&formatoutput)->len, strval, lenval);
    (&formatoutput)->len += lenval;
    (&formatoutput)->data[(&formatoutput)->len] = '\0';

    /* // TestStart: Check current value of formatoutput */
    /* text* check = cstring_to_text_with_len((&formatoutput)->data, (&formatoutput)->len); */
    /* PG_RETURN_TEXT_P(check); */
    /* // TestEnd */

    length = 0;
    resetStringInfo(&key);
    resetStringInfo(&output);
    tracker = cp;
    /* continue; */
  }
  length = output.len;

  // enlarge formatoutput to fit the length of the output until the value and the value
  enlargeStringInfo(&formatoutput, length);

  // copy the last part of the input string to the format string
  memcpy((&formatoutput)->data + (&formatoutput)->len, (&output)->data, length);

  // increment formatoutput length by the amount of the last part of the input string
  (&formatoutput)->len += length;

  (&formatoutput)->data[(&formatoutput)->len] = '\0';

  /* // TestStart: Check current value of formatoutput */
  /* text* check = cstring_to_text_with_len((&formatoutput)->data, (&formatoutput)->len); */
  /* PG_RETURN_TEXT_P(check); */
  /* // TestEnd */

  result = cstring_to_text_with_len(formatoutput.data, formatoutput.len);

  pfree(key.data);
  pfree(output.data);
  pfree(formatoutput.data);

  PG_RETURN_TEXT_P(result);
}
