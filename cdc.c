#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>
//
#include <gmp.h>

mpf_t    *stack;
size_t   stack_size = 0;
unsigned precision  = 2;
bool     running    = true;

void error(const char *msg) { printf("cdc: %s\n", msg); }

void print(const mpf_t a) { gmp_printf("%.*Ff\n", precision, a); }

void clear()
{
    if(stack_size == 0) error("stack already cleared");
    else
    {
        for(mpf_t *i = stack + stack_size - 1; i >= stack; --i) mpf_clear(*i);
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
    mpf_clear(stack[stack_size - 1]);
    if(stack_size == 1)
    {
        --stack_size;
        free(stack);
    }
    else
    {
        --stack_size;
        mpf_t *new_stack = realloc(stack, sizeof(mpf_t) * stack_size);
        if(new_stack == NULL) error("stack allocation error when popping");
        else stack = new_stack;
    }
}

void push()
{
    mpf_t *new_stack;
    if(++stack_size == 1) new_stack = malloc(sizeof(mpf_t));
    else                  new_stack = realloc(stack, sizeof(mpf_t) * stack_size);
    if(new_stack == NULL) error("stack allocation error when pushing");
    else
    {
        stack = new_stack;
        mpf_init(stack[stack_size - 1]);
    }
}

mpf_t *peek_at(size_t offset) { return &stack[(stack_size - 1) - offset]; }
mpf_t *peek()                 { return peek_at(0); }

void print_top()
{
    if(check_empty() == 0) print(*peek());
}

void dump()
{
    for(mpf_t *i = stack + (stack_size - 1); i >= stack; --i) print(*i);
}

void print_size() { printf("%zd\n", stack_size); }

void duplicate_top()
{
    if(check_empty() == 0)
    {
        push();
        mpf_set(*peek(), *peek_at(1));
    }
}

void apply_unary(void (*f)(mpf_t, const mpf_t))
{
    if(check_empty() == 0)
    {
        mpf_t op;
        mpf_init(op);
        mpf_set(op, *peek());
        pop();
        push();
        (*f)(*peek(), op);
        mpf_clear(op);
    }
}

void apply_binary(void (*f)(mpf_t, const mpf_t, const mpf_t))
{
    if(check_empty() == 0)
    {
        mpf_t op2;
        mpf_init(op2);
        mpf_set(op2, *peek());
        pop();
        if(check_empty() == 0)
        {
            mpf_t op1;
            mpf_init(op1);
            mpf_set(op1, *peek());
            pop();
            push();
            (*f)(*peek(), op1, op2);
            mpf_clear(op1);
        }
        mpf_clear(op2);
    }
}

void wrap_div(mpf_t rop, const mpf_t op1, const mpf_t op2)
{
    if(mpf_cmp_ui(op2, 0) == 0)
    {
        error("divide by zero");
        return;
    }
    mpf_div(rop, op1, op2);
}

void wrap_sqrt(mpf_t rop, const mpf_t op)
{
    if(mpf_cmp_ui(op, 0) < 0)
    {
        error("negative square root");
        return;
    }
    mpf_sqrt(rop, op);
}

void wrap_pow(mpf_t rop, const mpf_t op1, const mpf_t op2)
{
    mpf_pow_ui(rop, op1, mpf_get_ui(op2));
}

void take_digits(const char **str)
{
    while(**str != '\0' && (isdigit(**str) ||
                           **str == 'e'    ||
                           **str == '.')) ++(*str);
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
            case '+': apply_binary(&mpf_add);  break;
            case '-': apply_binary(&mpf_sub);  break;
            case '*': apply_binary(&mpf_mul);  break;
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
                if(end != i)
                {
                    size_t len = (end - i) + 1;
                    char *str = malloc(sizeof(char) * len);
                    mpf_t val;
                    mpf_init(val);
                    strncpy(str, i, len - 1);
                    str[len - 1] = '\0';
                    if(mpf_set_str(val, str, 10) != 0)
                    {
                        error("error while converting input to number");
                    }
                    free(str);
                    i = end - 1;
                    if(read_precision)
                    {
                        precision = mpf_get_ui(val);
                        mpf_set_default_prec(precision);
                    }
                    else
                    {
                        if(negate) mpf_neg(val, val);
                        push();
                        mpf_set(*peek(), val);
                    }
                    mpf_clear(val);
                }
                else error("invalid command");
            } break;
        }
    }
}

int main()
{
    mpf_set_default_prec(precision);
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
