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

void clear()
{
    if(stack_size == 0) error("stack already cleared");
    else
    {
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
    if(--stack_size == 0) free(stack);
    else
    {
        Num *new_stack = realloc(stack, sizeof(Num) * stack_size);
        if(new_stack == NULL) error("stack allocation error when popping");
        else stack = new_stack;
    }
}

void push(Num val)
{
    Num *new_stack;
    if(++stack_size == 1) new_stack = malloc(sizeof(Num));
    else                  new_stack = realloc(stack, sizeof(Num) * stack_size);
    if(new_stack == NULL) error("stack allocation error when pushing");
    else
    {
        stack = new_stack;
        stack[stack_size - 1] = val;
    }
}

Num peek() { return stack[stack_size - 1]; }

void print_top()
{
    if(check_empty() == 0) print(peek());
}

void dump()
{
    for(Num *i = stack + (stack_size - 1); i >= stack; --i) print(*i);
}

void print_size() { print(stack_size); }

void duplicate_top()
{
    if(check_empty() == 0) push(peek());
}

void apply_unary(Num (*f)(Num))
{
    if(check_empty() == 0)
    {
        Num a = peek();
        pop();
        push((*f)(a));
    }
}

void apply_binary(Num (*f)(Num, Num))
{
    if(check_empty() == 0)
    {
        Num b = peek();
        pop();
        if(check_empty() == 0)
        {
            Num a = peek();
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
    for(size_t i = 0; i < line_size - 1; ++i)
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
            case '=': print_top();             break;
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
                if(read_precision) precision = val;
                else
                {
                    if(negate) val = -val;
                    push(val);
                }
            }
            else error("invalid command");
            break;
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
