#include "linux/string.h"
#include "zxic_common.h"
#include "dpp_cmd_shell.h"

#ifndef whitespace
#define whitespace(c)       (((c) == ' ') || ((c) == '\t'))
#endif

#define DPP_CMD_ARG_NUM_MAX (15)

ZXIC_UINT32 dpp_cmd_help(ZXIC_VOID)
{
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 list_index = 0;

    list_index = sizeof(dpp_commands) / sizeof(dpp_commands[0]);

    for (i = 0; i < list_index; i++)
    {
        ZXIC_COMM_PRINT("%-40s | %s\n", dpp_commands[i].name, dpp_commands[i].doc);
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_cmd_atoi(ZXIC_CHAR* str)
{
    ZXIC_UINT32 n = 0;
    ZXIC_SINT32 rc = 0;

    if(str == NULL)
    {
       return 0x0;
    }

    if((str[0] == '0') && (str[1] == 'x'))
    {
        rc = sscanf(str, "0x%x", &n);
    }
    else if((str[0] == '0') && (str[1] == 'X'))
    {
        rc = sscanf(str, "0X%x", &n);
    }
    else
    {
        rc = sscanf(str, "%u", &n);
    }

    if (rc < 0)
    {
        return 0;
    }

    return n;
}

DPP_COMMAND* dpp_cmd_find(ZXIC_CHAR* name)
{
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 list_index = 0;

    list_index = sizeof(dpp_commands) / sizeof(dpp_commands[0]);

    for (i = 0; i < list_index; i++)
    {
        if (strcmp(name, dpp_commands[i].name) == 0)
        {
            return (&dpp_commands[i]);
        }
    }

    return ((DPP_COMMAND *)NULL);
}

ZXIC_UINT32 dpp_cmd_strtok(ZXIC_CHAR *str, ZXIC_CHAR** arg_v, ZXIC_UINT32* arg_num)
{
    ZXIC_CHAR* p_tok = NULL;
    ZXIC_CONST ZXIC_CHAR* delim = " ();\t";
    ZXIC_UINT32 i = 0;

    ZXIC_COMM_CHECK_POINT(str);
    ZXIC_COMM_CHECK_POINT(arg_v);
    ZXIC_COMM_CHECK_POINT(arg_num);

    ZXIC_COMM_MEMSET(arg_v, 0, DPP_CMD_ARG_NUM_MAX * sizeof(ZXIC_CHAR*));

    p_tok = strsep(&str, delim);
    ZXIC_COMM_CHECK_POINT(p_tok);

    arg_v[0] = p_tok;

    i = 1;
    while (p_tok && (i < DPP_CMD_ARG_NUM_MAX))
    {
        p_tok = strsep(&str, delim);
        if (p_tok == NULL)
        {
            break;
        }
        arg_v[i++] = p_tok;
    }

    *arg_num = i;

    return DPP_OK;
}

ZXIC_CHAR* dpp_cmd_trim(ZXIC_CHAR* line)
{
    ZXIC_CHAR* s;
    ZXIC_CHAR* t;

    ZXIC_COMM_CHECK_POINT_RETURN_NULL(line);

    for (s = line; whitespace(*s); s++)
    ;

    if (*s == 0)
    {
        return (s);
    }

    t = s + strlen(s) - 1;
    while ((t > s) && whitespace(*t))
    t--;
    *++t = '\0';

    return s;
}

ZXIC_UINT32 dpp_cmd_exec(ZXIC_CHAR* line)
{
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 arg_num = 0;
    ZXIC_UINT32 len = 0;
    ZXIC_CHAR* word = 0;
    ZXIC_CHAR* arg_v[DPP_CMD_ARG_NUM_MAX] = {0};
    DPP_COMMAND* command = NULL;

    ZXIC_UINT32 (*func0)(ZXIC_VOID);
    ZXIC_UINT32 (*func1)(ZXIC_UINT32);
    ZXIC_UINT32 (*func2)(ZXIC_UINT32, ZXIC_UINT32);
    ZXIC_UINT32 (*func3)(ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32);
    ZXIC_UINT32 (*func4)(ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32);
    ZXIC_UINT32 (*func5)(ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32);
    ZXIC_UINT32 (*func6)(ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32);
    ZXIC_UINT32 (*func7)(ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32);
    ZXIC_UINT32 (*func8)(ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32);
    ZXIC_UINT32 (*func9)(ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32);
    ZXIC_UINT32 (*func10)(ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32);
    ZXIC_UINT32 (*func11)(ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32,
                          ZXIC_UINT32);
    ZXIC_UINT32 (*func12)(ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32, ZXIC_UINT32,
                          ZXIC_UINT32, ZXIC_UINT32);

    ZXIC_COMM_CHECK_POINT(line);

    len = ZXIC_COMM_STRLEN(line);
    if(0 == len)
    {
        ZXIC_COMM_PRINT("len is 0.\n");
        return DPP_OK;
    }

    i = 0;
    while(line[i % (len + 1)] && whitespace(line[i % (len + 1)]))
    {
        i++;
    }
    word = line + i;

    while(line[i % (len + 1)] && !whitespace(line[i % (len + 1)]))
    {
        i++;
    }

    if(line[i % (len + 1)])
    {
        line[i++] = '\0';
    }

    command = dpp_cmd_find(word);
    ZXIC_COMM_CHECK_POINT(command);
    ZXIC_COMM_CHECK_POINT(command->func);

    while(whitespace(line[i % (len + 1)]))
    {
        i++;
    }

    word = line + i;

    rc = dpp_cmd_strtok(word, arg_v, &arg_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_cmd_strtok");
    ZXIC_COMM_CHECK_INDEX(arg_num, 0, DPP_CMD_ARG_NUM_MAX);

    switch (arg_num)
    {
        case 0:
        {
            func0 = command->func;
            ((*(func0)) ());
            break;
        }
        case 1:
        {
            func1 = command->func;
            ((*(func1)) (dpp_cmd_atoi(arg_v[0])));
            break;
        }
        case 2:
        {
            func2 = command->func;
            ((*(func2)) (dpp_cmd_atoi(arg_v[0]), dpp_cmd_atoi(arg_v[1])));
            break;
        }
        case 3:
        {
            func3 = command->func;
            ((*(func3)) (dpp_cmd_atoi(arg_v[0]), dpp_cmd_atoi(arg_v[1]), dpp_cmd_atoi(arg_v[2])));
            break;
        }
        case 4:
        {
            func4 = command->func;
            ((*(func4)) (dpp_cmd_atoi(arg_v[0]), dpp_cmd_atoi(arg_v[1]), dpp_cmd_atoi(arg_v[2]), dpp_cmd_atoi(arg_v[3])));
            break;
        }
        case 5:
        {
            func5 = command->func;
            ((*(func5)) (dpp_cmd_atoi(arg_v[0]), dpp_cmd_atoi(arg_v[1]), dpp_cmd_atoi(arg_v[2]), dpp_cmd_atoi(arg_v[3]), dpp_cmd_atoi(arg_v[4])));
            break;
        }
        case 6:
        {
            func6 = command->func;
            ((*(func6)) (dpp_cmd_atoi(arg_v[0]), dpp_cmd_atoi(arg_v[1]), dpp_cmd_atoi(arg_v[2]), dpp_cmd_atoi(arg_v[3]), dpp_cmd_atoi(arg_v[4]),
                                dpp_cmd_atoi(arg_v[5])));
            break;
        }
        case 7:
        {
            func7 = command->func;
            ((*(func7)) (dpp_cmd_atoi(arg_v[0]), dpp_cmd_atoi(arg_v[1]), dpp_cmd_atoi(arg_v[2]), dpp_cmd_atoi(arg_v[3]), dpp_cmd_atoi(arg_v[4]),
                                 dpp_cmd_atoi(arg_v[5]), dpp_cmd_atoi(arg_v[6])));
            break;
        }
        case 8:
        {
            func8 = command->func;
            ((*(func8)) (dpp_cmd_atoi(arg_v[0]), dpp_cmd_atoi(arg_v[1]), dpp_cmd_atoi(arg_v[2]), dpp_cmd_atoi(arg_v[3]), dpp_cmd_atoi(arg_v[4]),
                                 dpp_cmd_atoi(arg_v[5]), dpp_cmd_atoi(arg_v[6]), dpp_cmd_atoi(arg_v[7])));
            break;
        }
        case 9:
        {
            func9 = command->func;
            ((*(func9)) (dpp_cmd_atoi(arg_v[0]), dpp_cmd_atoi(arg_v[1]), dpp_cmd_atoi(arg_v[2]), dpp_cmd_atoi(arg_v[3]), dpp_cmd_atoi(arg_v[4]),
                                 dpp_cmd_atoi(arg_v[5]), dpp_cmd_atoi(arg_v[6]), dpp_cmd_atoi(arg_v[7]), dpp_cmd_atoi(arg_v[8])));
            break;
        }
        case 10:
        {
            func10 = command->func;
            ((*(func10)) (dpp_cmd_atoi(arg_v[0]), dpp_cmd_atoi(arg_v[1]), dpp_cmd_atoi(arg_v[2]), dpp_cmd_atoi(arg_v[3]), dpp_cmd_atoi(arg_v[4]),
                                 dpp_cmd_atoi(arg_v[5]), dpp_cmd_atoi(arg_v[6]), dpp_cmd_atoi(arg_v[7]), dpp_cmd_atoi(arg_v[8]), dpp_cmd_atoi(arg_v[9])));
            break;
        }
        case 11:
        {
            func11 = command->func;
            ((*(func11)) (dpp_cmd_atoi(arg_v[0]), dpp_cmd_atoi(arg_v[1]), dpp_cmd_atoi(arg_v[2]), dpp_cmd_atoi(arg_v[3]), dpp_cmd_atoi(arg_v[4]),
                                 dpp_cmd_atoi(arg_v[5]), dpp_cmd_atoi(arg_v[6]), dpp_cmd_atoi(arg_v[7]), dpp_cmd_atoi(arg_v[8]), dpp_cmd_atoi(arg_v[9]),
                                 dpp_cmd_atoi(arg_v[10])));
            break;
        }
        case 12:
        {
            func12 = command->func;
            ((*(func12)) (dpp_cmd_atoi(arg_v[0]), dpp_cmd_atoi(arg_v[1]), dpp_cmd_atoi(arg_v[2]), dpp_cmd_atoi(arg_v[3]), dpp_cmd_atoi(arg_v[4]),
                                 dpp_cmd_atoi(arg_v[5]), dpp_cmd_atoi(arg_v[6]), dpp_cmd_atoi(arg_v[7]), dpp_cmd_atoi(arg_v[8]), dpp_cmd_atoi(arg_v[9]),
                                 dpp_cmd_atoi(arg_v[10]),dpp_cmd_atoi(arg_v[11])));
            break;
        }
        default:
        {
            ZXIC_COMM_PRINT("err [arg_num:%d] oversize.\n", arg_num);
            break;
        }
        
    }
  
    return DPP_OK;
}
