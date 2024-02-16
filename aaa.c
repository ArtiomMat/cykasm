#include <stdio.h>
#include <stdlib.h>

/**
 * Artiom's Assembly Assembler
 * This is nothing more than a weird assembler for a nonexisting architecture.
 * I am also confident that the way I made the device communication is either impractically slow or perhaps even impossible with hardware, but atleast it's fun :).
*/

#define MEGABYTE (1<<20)

// The second page
#define CMDS_START_PTR ((unsigned long long)memory + 1<<16)

enum
{
  CMD_ZE, // Zero, end of commands
  CMD_MO, // Move

  CMD_NO, // Not
  CMD_AN, // And
  CMD_XN, // Xand
  CMD_NN, // Nand
  CMD_OR, // Or
  CMD_XR, // Xor
  CMD_NR, // Nor

  CMD_SL, // Shift left
  CMD_SR, // Shift right

  CMD_AD, // Add
  CMD_SU, // Subtract
  CMD_DI, // Unsigned divide
  CMD_ID, // Signed divide
  CMD_MU, // Unsigned multiply
  CMD_IM, // Signed multiply

  CMD_IF, // If 0 skip next command
  CMD_NF, // If !0 skip next command
  CMD_GO, // Go to a location

  CMD_PA, // Set page of memory, page size is 2^16

  CMD_PU, // Push
  CMD_PO, // Pop

  CMD_DE, // Puts into an argument how many bytes the device wants to input
  CMD_IN, // Input from device
  CMD_OU, // Output to device

  CMD_RE, // Rest milliseconds
};

/**
 * An array of argument number for each command type
*/
int const args_for_cmd[] =
{
  2,
  
  1,
  2,
  2,
  2,
  2,
  2,
  2,

  2,
  2,

  2,
  2,
  2,
  2,
  2,
  2,

  1,
  1,
  1,

  1,

  1,
  1,

  2,
  2,
  2,

  1,
};

typedef struct
{
  short type;
  struct cmd_arg
  {
    short is_ref : 1;
    short is_reg : 1;
    short value;
  } args[2];
} cmd_t;

typedef struct
{
  /**
   * Considered a register. A "page" is a 2^16 memory buffer 
  */
  unsigned short page;
  /**
   * 'Considered' a register. Pointer to the command in the `memory` buffer.
   * Note, in reality to be consistent it would be split into cmd_page and cmd_ref, but that would be slower, and the code never accesses the cmd register.
  */
  cmd_t* cmd_p;
  /**
   * An array of the general registers that the code can access.
  */
  union
  {
    signed short s;
    unsigned short u;
  } regs[10];
} cpu_t;

cpu_t cpu = {0};

/**
 * Just args[1]
*/
char* fp = 0;

/**
 * For compilation, global because multiple functions use the line number.
*/
int line = 1;

/**
 * A type used during compilation. A singly-linked list type.
*/
typedef struct lbl
{
  struct lbl* next;
  const char* word; // Pointer in the cleaned up string
  int cmd_i;
} lbl_t;

/**
 * The list itself
*/
lbl_t* labels = 0;


short memory[16*MEGABYTE] __attribute__ ((aligned (8))) = {0};

/**
 * Converts a ref type value from `cmd_t` into a memory pointer within the `memory` buffer using `page` register too.
*/
short*
ref_to_ptr(unsigned short ref)
{
  return &(memory[(unsigned)ref + ((unsigned)cpu.page << 16)]);
}

int
is_space(char c)
{
  // return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\b';
  return c <= 32;
}

/**
 * Cleans up an input string for further compilation. Returns a `malloc()`ed string so make sure to `free()` it to avoid memory leaks.
 * Removes extra spaces + lower cases everything.
*/
char*
clean_code(char* in)
{
  char* out;

  int out_n = 0;
  int saw_space = 1;
  for (int i = 0; in[i]; i++)
  {
    char c = in[i];

    if (is_space(c))
    {
      // This logic essentially allows for as many newlines as there were between two words
      if (c == '\n')
      {
        out_n++;
      }
      else if (!saw_space)
      {
        out_n++;
      }
      saw_space = 1;
    }
    else
    {
      saw_space = 0;
      out_n++;
      
      // Lower case here
      if (c >= 'A' && c <= 'Z')
      {
        in[i] = c+32;
      }
    }
  }

  if (saw_space) // Means that there was trailing spaces and we added one
  {
    out_n--;
  }

  out = malloc(out_n+1); // +1 For the null terminator
  
  saw_space = 1;
  for (int i = 0, j = 0; in[i] && j <= out_n; i++)
  {
    char c = in[i];

    if (is_space(c))
    {
      if (c == '\n')
      {
        out[j++] = '\n';
      }
      else if (!saw_space)
      {
        out[j++] = ' ';
      }
      saw_space = 1;
    }
    else
    {
      saw_space = 0;
      out[j++] = c;
    }
  }

  out[out_n] = 0;

  return out;
}

/**
 * Converts a 'word', which is a regular ass string but any <=32 ascii is considered a terminator.
 * `word` is a pointer to first character of the word, can start with '-' for negative numbers, can be empty too.
 * If the word has any content returns the number the word represents, if the number is invalid `__UINT32_MAX__` is returned.
*/
int
wtoi(const char* word)
{
  int num = 0;
  int plus = 1;

  if (word[0] == '-')
  {
    plus = 0;
    word++;
  }

  if (is_space(word[0]))
  {
    return __UINT32_MAX__;
  }
  
  int digits_n;
  for (digits_n = 0; !is_space(word[digits_n]); digits_n++)
  {
    if (word[digits_n] > '9' || word[digits_n] < '0')
    {
      return __UINT32_MAX__;
    }
  }

  {
    int factor = 1;
    for (int i = digits_n-1; i >= 0; i--) {
      num += (word[i] - '0') * factor;
      factor *= 10;
    }
  }

  if (plus)
  {
    return num;
  }
  return -num;
}

/**
 * Compare two words, check `wtoi()` for what a word is.
 * Returns 1 if the words are identical, 0 if not.
*/
int
wrdcmp(const char* a, const char* b)
{
  int i;
  for (i = 0; !is_space(a[i]); i++)
  {
    if (a[i] != b[i])
    {
      return 0;
    }
  }
  return is_space(b[i]); // b must end too at i
}

void
err(char* str)
{
  printf("%s:%d: %s.\n", fp, line, str);
}

/**
 * Compiles an argument of a command(e.g. mo arg0 arg1)
 * Returns 0 if the argument is invalid, 1 if it was syntatically valid.
*/
int
compile_arg(const char* start, struct cmd_arg* p)
{
  if (start[0] == '?') // If the argument represents a reference(memory location)
  {
    p->is_ref = 1;
    start++;
  }

  if (start[0] == 'r' || start[0] == 'R') // A register
  {
    if (!is_space(start[2]))
    {
      err("Invalid register");
      return 0;
    }
    p->is_reg = 1;
    p->value = start[1]-'0';
    if (p->value < 0 || p->value > sizeof(cpu.regs)/sizeof(cpu.regs[0]))
    {
      err("Invalid register");
      return 0;
    }
  }
  else if (start[0] == '.')
  {
    int found = 0;
    for (lbl_t* l = labels; l; l = l->next)
    {
      if (wrdcmp(l->word, start+1))
      {
        p->value = l->cmd_i * (sizeof(cmd_t) / sizeof(short));
        found = 1;
        break;
      }
    }
    if (!found)
    {
      err("Label not found");
    }
    return 0;
  }
  else if (start[0] >= '0' && start[0] <= '9') // A regular value
  {
    int value = wtoi(start);
    if (value == __UINT32_MAX__)
    {
      return 0;
    }
    p->value = value;
  }
  else
  {
    err("Invalid argument format");
  }
  return 1;
}


/**
 * Returns the enum type of the command according to a=str[0] b=str[1] characters.
 * Returns -1 if invalid.
*/
short wtoc(char a, char b) {
  if (a == 'a')
  {
    if (b == 'd')
    {
      return CMD_AD;
    }
  }
  if (a == 'g')
  {
    if (b == 'o')
    {
      return CMD_GO;
    }
  }
  else if (a == 'm')
  {
    if (b == 'o')
    {
      return CMD_MO;
    }
    else if (b == 'u')
    {
      return CMD_MU;
    }
  }
  else if (a == 'd')
  {
    if (b == 'e')
    {
      return CMD_DE;
    }
  }
  else if (a == 'i')
  {
    if (b == 'f')
    {
      return CMD_IF;
    }
    else if (b == 'n')
    {
      return CMD_IN;
    }
  }
  else if (a == 'o')
  {
    if (b == 'u')
    {
      return CMD_OU;
    }
  }

  return -1;
}


/**
 * Returns how many characters to add for the next word
*/
int
skip_word_chars(const char* str)
{
  int i;
  for (i = 0; !is_space(str[i]); i++) // Skip none spaces
  {}

  if (str[i] == ' ') // If there were no new lines then just return this
  {
    return i+1;
  }

  for (; str[i] == '\n'; i++) // Skip newline characters
  {
    line++;
  }

  return i; // Also works for null terminator
}

/**
 * Returns how many characters to add for the next line
*/
int
skip_line_chars(const char* str)
{
  int i;
  for (i = 0; str[i] && str[i] != '\n'; i++)
  {}

  for (; str[i] == '\n'; i++) // Skip newline characters
  {
    line++;
  }

  return i; // Also works for null terminator
}

/**
 * Precompiles the code and sets up the
 * Additionally it performs some checks: if the commands are valid, if a command has enough arguments or if the file suddenly ended, if labels already exist.
 * Only accepts a cleaned string!
 * Returns 1 on success, 0 on error.
*/
int
precompile_labels(const char* str)
{
  cmd_t* cmd_p = CMDS_START_PTR; // Pointer of the current command we are writing to
  
  // Firstly, we pre-compile the labels
  while (str[0])
  {
    if (str[0] == ';') // New line?
    {
      str += skip_line_chars(str);
    }
    else if (str[0] == '.') // If we have a label we add it to the list
    {
      int found = 0;
      
      for (lbl_t* l = labels; l; l = l->next)
      {
        if (wrdcmp(l->word, str+1))
        {
          found = 1;
          break;
        }
      }

      if (found)
      {
        err("Label already exists");
        return 0;
      }
      else
      {
        lbl_t* l = malloc(sizeof (lbl_t));
        l->cmd_i = cmd_p - ((cmd_t*)memory);
        l->next = labels;
        l->word = str+1;
        labels = l;
      }

      str += skip_word_chars(str);
    }
    else
    {
      cmd_p->type = wtoc(str[0], str[1]);

      if (!is_space(str[2]) || cmd_p->type == -1)
      {
        err("Invalid command");
        return 0;
      }
      str += skip_word_chars(str);

      for (int i = 0; i < args_for_cmd[cmd_p->type]; i++)
      {
        // Check if the string ended, hence there are not enough arguments!
        if (str[0] == 0)
        {
          err("Not enough arguments");
          return 0;
        }
        str += skip_word_chars(str);
      }

      cmd_p++;
    }
  }

  line = 1; // Reset line, skip_word_chars() adds line numbers automatically
  return 1;
}

/**
 * Compiles the code and pushes it into the memory
 * Only accepts a cleaned string!
 * Returns 1 on success, 0 on error.
*/
int
compile_and_push(const char* str)
{
  cmd_t* cmd_p = CMDS_START_PTR; // Pointer of the current command we are writing to

  if (!precompile_labels(str))
  {
    return 0;
  }

  while (str[0])
  {
    if (str[0] == ';') // New line?
    {
      str += skip_line_chars(str);
    }
    else if (str[0] == '.') // If we have a label skip it, we added it already
    {
      str += skip_word_chars(str);
    }
    else
    {
      cmd_p->type = wtoc(str[0], str[1]);

      // We already validate the command in the precompilation
      str += skip_word_chars(str);
      printf("%d ", args_for_cmd[cmd_p->type]);
      for (int i = 0; i < args_for_cmd[cmd_p->type]; i++)
      {
        // Checked if enough arguments in precompilation
        compile_arg(str, &cmd_p->args[i]);
        str += skip_word_chars(str);
      }

      printf
      (
        "%d %c%c%d %c%c%d\n",
        cmd_p->type,
        cmd_p->args[0].is_ref?'@':' ', cmd_p->args[0].is_reg?'r':' ', cmd_p->args[0].value,
        cmd_p->args[1].is_ref?'@':' ', cmd_p->args[1].is_reg?'r':' ', cmd_p->args[1].value
      );
      
      cmd_p++;
    }
  }

  // Now cleanup all the compilation shit
  for (lbl_t* l = labels; l;)
  {
    lbl_t* fl = l;
    l = fl->next;
    free(fl);
  }

  return 1;
}

short
evaluate_get(struct cmd_arg* arg)
{
  short value;
  if (arg->is_reg)
  {
    value = cpu.regs[arg->value].s;
  }
  else
  {
    value = arg->value;
  }

  if (arg->is_ref) // If there is a reference flag then we actually take the value in memory!
  {
    value = *ref_to_ptr(value);
  }

  return value;
}

void
evaluate_set(struct cmd_arg* arg, short x)
{
  short* ptr;
  if (arg->is_reg)
  {
    ptr = &cpu.regs[arg->value];

    if (arg->is_ref) // If it's a reference we do the same procedure that we do for regular numbers
    {
      goto _is_ref;
    }
    else // If not then we just set the register value
    {
      *ptr = x;
    }
  }
  else
  {
    ptr = arg->value;

    if (arg->is_ref) // If there is a reference flag then we actually take the value in memory!
    {
      _is_ref:
      ptr = *ref_to_ptr(*ptr);
    }

    *ref_to_ptr(*ptr) = x;
  }

}

/**
 * Just runs the memory!
*/
void
run()
{
  cpu.cmd_p = CMDS_START_PTR;
  
  for (cmd_t* cmd_p = (cmd_t*) memory; cmd_p->type; cmd_p++)
  {
    switch (cmd_p->type)
    {
      case CMD_MO:
      evaluate_set(&cmd_p->args[0], evaluate_get(&cmd_p->args[1]));
      printf("%d\n", cpu.regs[1].s);
      break;

      case CMD_ZE:
    }
  }
  return 0;
}

int
main(int args_n, char** args)
{
  if (args_n < 2)
  {
    puts("Nah we need the file.");
    return 1;
  }

  fp = args[1];

  FILE* f = fopen(fp, "rb");
  if (!f)
  {
    puts("File not found.");
    return 1;
  }

  char* fstr = 0;

  // Setup fstr
  {
    fseek(f, 0, SEEK_END);
    long fs = ftell(f);
    rewind(f);
    
    char* str = malloc(fs+1);
    fread(str, 1, fs, f);
    str[fs] = 0;

    fstr = clean_code(str);
    free(str);
  }

  // puts(fstr);
  if (!compile_and_push(fstr))
    return 1;
  


  return 0;
}
