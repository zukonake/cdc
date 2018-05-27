#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>
//
#include <mpfr.h>

mpfr_t   *stack;
size_t    stack_size = 0;
unsigned  precision  = 2;
bool      running    = true;

void error(const char *msg) { printf("cdc: %s\n", msg); }

void print(const mpfr_t a) { mpfr_printf("%.*Rf\n", precision, a); }

void clear()
{
    if(stack_size == 0) error("stack already cleared");
    else
    {
        for(mpfr_t *i = stack + stack_size - 1; i >= stack; --i) mpfr_clear(*i);
        free(stack);
        stack_size = 0;
    }
}

int check_empty()
{
    if(stack_size == 0)
    {
        error("stack is empty");
        return -1;
    }
    return 0;
}

void pop()
{
    mpfr_clear(stack[stack_size - 1]);
    if(stack_size == 1)
    {
        --stack_size;
        free(stack);
    }
    else
    {
        --stack_size;
        mpfr_t *new_stack = realloc(stack, sizeof(mpfr_t) * stack_size);
        if(new_stack == NULL) error("stack allocation error when popping");
        else stack = new_stack;
    }
}

void push()
{
    mpfr_t *new_stack;
    if(++stack_size == 1) new_stack = malloc(sizeof(mpfr_t));
    else                  new_stack = realloc(stack, sizeof(mpfr_t) * stack_size);
    if(new_stack == NULL) error("stack allocation error when pushing");
    else
    {
        stack = new_stack;
        mpfr_init(stack[stack_size - 1]);
    }
}

mpfr_t *peek_at(size_t offset) { return &stack[(stack_size - 1) - offset]; }
mpfr_t *peek()                 { return peek_at(0); }

void print_top()
{
    if(check_empty() == 0) print(*peek());
}

void dump()
{
    for(mpfr_t *i = stack + (stack_size - 1); i >= stack; --i) print(*i);
}

void print_size() { printf("%zd\n", stack_size); }

void duplicate_top()
{
    if(check_empty() == 0)
    {
        push();
        mpfr_set(*peek(), *peek_at(1), MPFR_RNDN);
    }
}

void apply_unary(int (*f)(mpfr_t, const mpfr_t, mpfr_rnd_t))
{
    if(check_empty() == 0)
    {
        mpfr_t op;
        mpfr_init(op);
        mpfr_set(op, *peek(), MPFR_RNDN);
        pop();
        push();
        (*f)(*peek(), op, MPFR_RNDN);
        mpfr_clear(op);
    }
}

void apply_binary(int (*f)(mpfr_t, const mpfr_t, const mpfr_t, mpfr_rnd_t))
{
    if(check_empty() == 0)
    {
        mpfr_t op2;
        mpfr_init(op2);
        mpfr_set(op2, *peek(), MPFR_RNDN);
        pop();
        if(check_empty() == 0)
        {
            mpfr_t op1;
            mpfr_init(op1);
            mpfr_set(op1, *peek(), MPFR_RNDN);
            pop();
            push();
            (*f)(*peek(), op1, op2, MPFR_RNDN);
            mpfr_clear(op1);
        }
        mpfr_clear(op2);
    }
}

int wrap_div(mpfr_t rop, const mpfr_t op1, const mpfr_t op2, mpfr_rnd_t rnd)
{
    if(mpfr_cmp_ui(op2, 0) == 0)
    {
        error("divide by zero");
        return 0;
    }
    return mpfr_div(rop, op1, op2, rnd);
}

int wrap_sqrt(mpfr_t rop, const mpfr_t op, mpfr_rnd_t rnd)
{
    if(mpfr_cmp_ui(op, 0) < 0)
    {
        error("negative square root");
        return 0;
    }
    return mpfr_sqrt(rop, op, rnd);
}

int wrap_pow(mpfr_t rop, const mpfr_t op1, const mpfr_t op2, mpfr_rnd_t rnd)
{
    return mpfr_pow(rop, op1, op2, rnd);
}

void take_digits(const char **str)
{
    while(**str != '\0' && (isdigit(**str) ||
                           **str == 'e'    ||
                           **str == '.')) ++(*str);
}

void parse_number(const char **start, const char *end, bool read_precision, bool negate)
{
    size_t len = (end - *start) + 1;
    char *str = malloc(sizeof(char) * len);
    mpfr_t val;
    mpfr_init(val);
    strncpy(str, *start, len - 1);
    str[len - 1] = '\0';
    if(mpfr_set_str(val, str, 10, MPFR_RNDN) != 0)
    {
        error("error while converting input to number");
    }
    free(str);
    *start = end - 1;
    if(read_precision)
    {
        precision = mpfr_get_ui(val, MPFR_RNDN);
        mpfr_set_default_prec(precision);
    }
    else
    {
        if(negate) mpfr_neg(val, val, MPFR_RNDN);
        push();
        mpfr_set(*peek(), val, MPFR_RNDN);
    }
    mpfr_clear(val);
}

void parse_line(const char *line, size_t line_size)
{
    bool read_precision = false;
    bool negate         = false;
    for(const char *i = line; i < line + line_size; ++i)
    {
        switch(*i)
        {
            case ' ':                          break;
            case '\n':                         break;
            case '+': apply_binary(&mpfr_add); break;
            case '-': apply_binary(&mpfr_sub); break;
            case '*': apply_binary(&mpfr_mul); break;
            case '/': apply_binary(&wrap_div); break;
            case '^': apply_binary(&wrap_pow); break;
            case 'v': apply_unary(&wrap_sqrt); break;
            case '=': print_top();             break;
            case 'k': read_precision = true;   break;
            case 'b': dump();                  break;
            case 's': print_size();            break;
            case 'd': duplicate_top();         break;
            case 'c': clear();                 break;
            case 'q': running = false;         break;
            case '_': negate  = true;          break;
            default:
            {
                const char *end = i;
                take_digits(&end);
                if(end != i) parse_number(&i, end, read_precision, negate);
                else error("invalid command");
            } break;
        }
    }
}

int main()
{
    char  *line     = NULL;
    size_t line_buf = 0;
    size_t line_size;
    while(running)
    {
        line_size  = getline(&line, &line_buf, stdin);
        parse_line(line, line_size);
    }
    if(stack_size != 0) clear();
    free(line);
    return 0;
}
