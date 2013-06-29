#include "contiki.h"
#include "window.h"

const char *month_name[] =
{
  "January", "February", "March", "April", "May",
  "June", "July", "August", "September",
  "November", "December"
};

static const char* english_name[] =
{
  "ZERO", "ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT",
  "NINE", "TEN", "ELEVEN", "TWELVE", "THIRTEEN", "FOURTEEN",
  "FIFTEEN", "SIXTEEN", "SEVENTEEN", "EIGHTEEN", "NINETEEN"
};

static const char* english_name_prefix[] =
{
    "TWENTY", "THIRTY", "FORTY", "FIFTY", "SIXTY",
    "SEVENTY", "EIGHTY", "NINTY"
};

const char *month_shortname[] = {
  "JAN","FEB","MAR","APR", "MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"
};

const char* week_shortname[] = {
  "SU", "MO", "TU", "WE", "TH", "FR", "SA"
};


// Only support number less than 100
const char* toEnglish(uint8_t number, char* buffer)
{
  if (number < 20)
  {
    return english_name[number];
  }

  // larger than 20
  strcpy(buffer, english_name_prefix[(number / 10) - 2]);
  if (number % 10 != 0)
  {
    strcat(buffer, " ");
    strcat(buffer, english_name[number % 10]);
  }

  return buffer;
}
