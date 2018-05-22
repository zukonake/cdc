#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>

typedef double Num;

Num     *stack;
size_t   stack_size = 0;
unsigned precision  = 4;
bool     running    = true;

void error(const char *msg) { printf("cdc: %s\n", msg); }
void print(Num a) { printf("%.*f\n", precision, a); }

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
    if(check_empty() != 0) return;
    stack = realloc(stack, sizeof(Num) * (--stack_size));
}

void push(Num val)
{
    stack = realloc(stack, sizeof(Num) * (++stack_size));
    stack[stack_size - 1] = val;
}

int peek(Num *val)
{
    if(check_empty() != 0) return -1;
    *val = stack[stack_size - 1];
    return 0;
}

void clear() { while(stack_size != 0) pop(); }

void print_top()
{
    Num val;
    if(peek(&val) == 0) print(val);
}

void dump()
{
    for(Num *i = stack + (stack_size - 1); i >= stack; --i) print(*i);
}

void print_size() { print(stack_size); }

void duplicate_top()
{
    Num a;
    if(peek(&a) == 0) push(a);
}

void apply_unary(Num (*f)(Num))
{
    Num a;
    if(peek(&a) == 0)
    {
        pop();
        push((*f)(a));
    }
}

void apply_binary(Num (*f)(Num, Num))
{
    Num b;
    if(peek(&b) == 0)
    {
        pop();
        Num a;
        if(peek(&a) == 0)
        {
            pop();
            push((*f)(a, b));
        }
    }
}

Num func_add(Num a, Num b) { return a + b; }
Num func_sub(Num a, Num b) { return a - b; }
Num func_mul(Num a, Num b) { return a * b; }
Num func_div(Num a, Num b)
{
    if(b == 0.0)
    {
        error("divide by zero");
        return 0;
    }
    return a / b;
}
Num func_sqrt(Num a)
{
    if(a < 0.0)
    {
        error("negative square root");
        return 0;
    }
    return sqrt(a);
}

void parse_line(char *line, size_t line_size)
{
    bool read_precision = false;
    bool negate         = false;
    for(size_t i = 0; i < line_size; ++i)
    {
        switch(line[i])
        {
            case ' ':                          break;
            case '+': apply_binary(&func_add); break;
            case '-': apply_binary(&func_sub); break;
            case '*': apply_binary(&func_mul); break;
            case '/': apply_binary(&func_div); break;
            case '^': apply_binary(&pow);      break;
            case 'v': apply_unary(&func_sqrt); break;
            case 'p': print_top();             break;
            case 'k': read_precision = true;   break;
            case 'b': dump();                  break;
            case 's': print_size();            break;
            case 'd': duplicate_top();         break;
            case 'c': clear();                 break;
            case 'q': running = false;         break;
            case '_': negate  = true;          break;
            default:
            if(isdigit(line[i]))
            {
                Num val = strtod(line + i, (char **)&i); //i is now an address to the next char
                i -= (size_t)line + 1;                   //convert address to iterator
                if(read_precision)
                {
                    precision = val;
                    read_precision = false;
                }
                else
                {
                    if(negate)
                    {
                        val    = -val;
                        negate = false;
                    }
                    push(val);
                }
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
    clear();
    free(line);
    return 0;
}
