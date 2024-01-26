#include <stdio.h>

enum {
  WORD_BEGIN,
  WORD_END,

  WORD_IF,
  WORD_ELSE,
  WORD_THEN,

  /*
    сука
    Deletes system32
  */
  WORD_DESTROY,
  /*
    блядь
    Such a bad error occured that the program has to just die
  */
  WORD_CRASH,
  /*
    пиздец
    An error occured, the code was not supposed to get here, exception equivalent.
  */
  WORD_ERROR,
};

const char* word_strs[] = {
  "начало",
  "конец",

  "если",
  "иначе",
  "то",

  "сука",
  "блядь"
  "пиздец",
};

char* str = 
"yesle () to konez\n"
"\n"
"\n"
"\n"
"\n";

int main(int args_n, char** args) {

}
