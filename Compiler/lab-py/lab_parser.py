import lex
#from pycallgraph import PyCallGraph
#from pycallgraph.output import GraphvizOutput
# -*- coding: UTF-8 -*-
dyd_list = []
file_name = ".\\lab2\\lab2.dyd"
base=file_name.split('.dyd')[0]
var_file_name = base + ".var"
pro_file_name = base + ".pro"
err_file_name = base + ".err"
dys_file_name = base + ".dys"

pos = 0
line = 1
begin_code = 1
end_code = 2
integer_code = 3
if_code = 4
then_code = 5
else_code = 6
function_code = 7
read_code = 8
write_code = 9
identifier_code = 10
constant_code = 11
left_braket_code=21
right_braket_code=22
semicolon_code=23
EOLN = 24
EOF = 25

var_define = 0
var_use = 1
formal_parameter = 1

err_TXT = ""
var_Table = []
var_count = 0
procedure_Table = []
procedure_count = 0
main_procedure_text = "main"
now_Procedure = main_procedure_text
procedure_level = 0


def end_files():
    with open(err_file_name,encoding="utf-8",mode="w") as err_file:
        print("err: \n{}write in file\n\n".format(err_TXT))
        err_file.write(err_TXT)

    with open(var_file_name,encoding="utf-8",mode="w") as var_file:
        print("var_file:")
        enum_string = "变量名 所属过程 分类 变量类型 变量层次 相对位置\n"
        for enum in var_Table:
            for i in range(0, len(enum)):
                enum_string += str(enum[i])+' '
            enum_string += '\n'
        var_file.write(enum_string)
        print("{} ".format(enum_string))
        print("write to file {}\n\n".format(var_file_name))

    with open(pro_file_name,encoding="utf-8",mode="w") as pro_file:
        print("process_file:")
        enum_string = "过程名 过程类型 层次 起始行数 结束行数\n"
        for enum in procedure_Table:
            for i in range(0, len(enum)):
                enum_string += str(enum[i])+' '
            enum_string += '\n'
            
        pro_file.write(enum_string)
        print("{} ".format(enum_string))
        print("write to file{}".format(var_file_name))
    if current_code() != EOF:
        err_print("规约完成，但函数未结束")

def init_files():
    dys_file  = open(dys_file_name, "w")
    with open(file_name, "r") as dyd_file:
        for line in dyd_file:
            line = line.strip()
            line.replace('\n', '')
            dyd_line = line.split(' ')
            dyd_line[1] = int(dyd_line[1])
            dyd_list.append(dyd_line)
            dys_file.write(line+'\n')
    dys_file.close()
    err_file = open(err_file_name, "w")
    print("1. Create {} Sucess~\n".format(err_file_name))
    err_file.close()
    pro_file = open(pro_file_name, "w")
    print("2. Create {} Sucess~\n".format(pro_file_name))
    pro_file.close()
    var_file = open(var_file_name, "w")
    print("3. Create {} Sucess~\n".format(var_file_name))
    var_file.close()



def current_code():
    global line
    global pos
    if dyd_list[pos][1] == EOLN:
        line += 1
        pos += 1
        return current_code()
    else:
        return dyd_list[pos][1]

def current_word():
    global line
    global pos
    if dyd_list[pos][1] == EOLN:
        line += 1
        pos += 1
        return current_word()
    else:
        return dyd_list[pos][0]


def advanced():
    current_code()
    global pos
    pos += 1

def now_more_one(temp = 1):
    if dyd_list[pos+temp][1] == EOLN:
        return now_more_one(temp+1)
    else:
        return dyd_list[pos+temp][1]





def err_print(err_info):
    global err_TXT
    err_TXT += err_info+" "+"Line:" + str(line)+'\n'
    #print "当前行为:" + str(line)

def main_procedure():
    """<程序>→<分程序>"""
    branch_procedure()
    return


def branch_procedure():
    """ <分程序>→begin <说明语句表>；<执行语句表> end"""
    if current_code() == begin_code:
        advanced()
        declare_statement_table()
        if current_code() == semicolon_code:
            advanced()
            exec_statement_table()
            #current_code()
            if current_code() == end_code:
                advanced()
            else:
                print("----{}-{}-{}-{}-".format(current_code(),pos,line,dyd_list[pos][0]))
                err_print("分程序错误，是否缺少 end")
        else:
            err_print("分程序错误，是否缺少 ;")
    else:
        err_print("分程序错误，是否缺少 begin")
    return

def declare_statement_table():
    """<说明语句表>→<说明语句>│<说明语句表> ；<说明语句>
    改写为
    <说明语句表>→<说明语句><说明语句表A>
    <说明语句表A>→;<说明语句><说明语句表A>│空
    """
    declare_statement()
    declare_statement_tableA()


def declare_statement_tableA():
    """<说明语句表A>→;<说明语句><说明语句表A>│空"""
    if current_code() == 23 and now_more_one() == integer_code:
        advanced()
        declare_statement()
        declare_statement_tableA()
    else:
        return

def declare_statement():
    """ <说明语句>→<变量说明>│<函数说明>"""
    if current_code() == integer_code:
        if now_more_one() == 7:
            func_declare()
        else:
            var_declare()
    else:
        err_print("说明语句出错,是否缺少integer")

def var_declare():
    """ <变量说明>→integer <变量>"""
    if current_code() == 3:
        advanced()
        var(var_define)
    else:
        err_print("变量说明出错，是否缺少integer")

def var(define_or_use,is_formal = 0):
    """<变量>→<标识符>"""
    global var_count
    if define_or_use == var_define:
        word = identifier()
        flag = True
        for var_table_line in var_Table:
            if word == var_table_line[0]:
                flag = False
        if flag:
            var_Table.append([word, now_Procedure, is_formal, "integer", procedure_level, var_count])
            var_count += 1
    elif define_or_use == var_use:
        word = identifier()
        use_word = False
        for var in var_Table:
            if var[0] == word:
                use_word = True
        for procedure in procedure_Table:
            if procedure[0] == word:
                use_word = True
        if not use_word:
            err_print("符号"+word+"无定义")
    for pro_table_line in procedure_Table:
        if pro_table_line[0] == now_Procedure:
            if pro_table_line[3] == -1:
                pro_table_line[3] = line
            if pro_table_line[4] < line:
                pro_table_line[4] = line+1


def identifier():
    """ <标识符>→<字母>│<标识符><字母>│ <标识符><数字>"""
    if current_code() == identifier_code:
        temp_word = current_word()
        advanced()
        return temp_word
    else:
        err_print("标识符出错")
        return None

def func_declare():
    # <函数说明>→integer function <标识符>（<参数>）；<函数体>
    global now_Procedure
    global procedure_level
    procedure_level += 1
    last_Procedure = now_Procedure
    if current_code() == integer_code:
        advanced()
        if current_code() == function_code:
            advanced()
            now_Procedure = identifier()
            flag = True

            for table_line in procedure_Table:
                if now_Procedure == table_line[0]:
                    flag = False
            if flag:
                procedure_Table.append([now_Procedure, "integer", procedure_level, -1, -1])

            if current_code() == left_braket_code:
                advanced()
                parameter()
                if current_code() == right_braket_code:
                    advanced()
                    if current_code() == semicolon_code:
                        advanced()
                        func_body()
                    else:
                        err_print("函数说明出错，缺少;")
                else:
                    err_print("函数说明出错，缺少)")
            else:
                err_print("函数说明出错，缺少(")
        else:
            err_print("函数说明出错，缺少function")
    else:
        err_print("函数说明出错，缺少integer")
    now_Procedure = last_Procedure
    procedure_level -= 1

def parameter():
    # <参数>→<变量>
    var(var_define, formal_parameter)
    return

def func_body():
    """ <函数体>→begin <说明语句表>；<执行语句表> end"""
    if current_code() == begin_code:
        advanced()
        declare_statement_table()
        if current_code() == semicolon_code:
            advanced()
            exec_statement_table()
            if current_code() == end_code:
                advanced()
            else:
                err_print("函数体出错，缺少end")
        else:
            err_print("函数体出错，缺少;")
    else:
        err_print("函数体错误，缺少begin")

def exec_statement_table():
    """
    # 左递归：<执行语句表>→<执行语句>；│<执行语句表>；<执行语句>；
    # 需要改写为
    # <执行语句表>→<执行语句>；<执行语句表A>；
    # <执行语句表A>→<执行语句>；<执行语句表A>│空
    """
    exec_statement()
    #if current_code==semicolon_code:
    #    advanced()
    exec_statement_tableA()
    #if current_code==semicolon_code:
    #    advanced()
    return

def exec_statement_tableA():
    """
    # <执行语句表A>→;<执行语句><执行语句表A>│空
    """
    if current_code() == semicolon_code and now_more_one() in [read_code,write_code, if_code,identifier_code]:
        advanced()
        exec_statement()
        exec_statement_tableA()
    else:
        return

def exec_statement():
    """
    # <执行语句>→<读语句>│<写语句>│<赋值语句>│<条件语句>
    """
    exec_state = current_code()
    if exec_state == read_code:
        read_statement()
    elif exec_state == write_code:
        write_statement()
    elif exec_state == if_code:
        condition_statement()
    elif exec_state == identifier_code:
        assign_statement()
    else:
        err_print("执行语句出错，不知道该走哪里")

def read_statement():
    """
    # <读语句>→read(<变量>)
    """
    if current_code() == 8:
        advanced()
        if current_code() == 21:
            advanced()
            var(var_use)
            if current_code() == 22:
                advanced()
            else:
                err_print("读语句出错")
        else:
            err_print("读语句出错")
    else:
        err_print("读语句出错")

def write_statement():
    """
    #<写语句>→write(<变量>)
    """
    if current_code() == 9:
        advanced()
        if current_code() == 21:
            advanced()
            var(var_use)
            if current_code() == 22:
                advanced()
                if current_code()==semicolon_code:
                    advanced()
                else:
                    err_file_name("写语句缺少 ;")
            else:
                err_print("写语句出错")
        else:
            err_print("写语句出错")
    else:
        err_print("写语句出错")


def assign_statement():
    """
    # <赋值语句>→<变量>:=<算术表达式>
    """
    var(var_use)
    if current_code() == 20:
        advanced()
        math_expression()
    else:
        err_print("赋值语句出错")

def math_expression():
    """
    # 左递归：<算术表达式>→<算术表达式>-<项>│<项>
    # 改写：<算术表达式>→<项><算术表达式A>
    #<算术表达式A>→-<项><算术表达式A>|空
    """
    item()
    math_expressionA()

def math_expressionA():
    """
    #<算术表达式A>→-<项><算术表达式A>|空
    """
    if current_code() == 18:
        advanced()
        item()
        math_expressionA()
    else:
        return

def item():
    """
    # 左递归：<项>→<项>*<因子>│<因子>
    # 改写<项>→<因子><项A>
    # <项A>→*<因子><项A>│空
    """
    factor()
    itemA()

def itemA():
    # <项A>→*<因子><项A>│空
    if current_code() == 19:
        advanced()
        factor()
        itemA()
    else:
        return

def factor():
    # <因子>→<变量>│<常数>│<函数调用>
    if current_code() == 11:
        constant()
    elif now_more_one() ==21:
        func_call()
    else:
        var(var_use)

def func_call():
    # <函数调用>→<标识符>(<算数表达式>)
    identifier()
    if current_code() == 21:
        advanced()
        math_expression()
        if current_code() == 22:
            advanced()
        else:
            err_print("函数调用出错")
    else:
        err_print("函数调用出错")

def constant():
    # <常数>→<无符号整数>
    unsigned_integer()

def unsigned_integer():
    if current_code() == 11:
        advanced()
    else:
        err_print("常数调用出错，遇到非数字")

def condition_statement():
    # <条件语句>→if<条件表达式>then<执行语句>else <执行语句>
    if current_code() == 4:
        advanced()
        condition_expression()
        if current_code() == 5:
            advanced()
            exec_statement()
            if current_code() == 6:
                advanced()
                exec_statement()
            else:
                err_print("条件语句出错")
        else:
            err_print("条件语句出错")
    else:
        err_print("条件语句出错")

def condition_expression():
    # <条件表达式>→<算术表达式><关系运算符><算术表达式>
    math_expression()
    relation_operator()
    math_expression()

def relation_operator():
    # <关系运算符> →<│<=│>│>=│=│<>
    if current_code() in [15, 14, 17, 16, 12, 13]:
        advanced()
    else:
        err_print("关系运算符出错")

if __name__ == '__main__':
    print("\n 先调用lex.py 创建dyd文件\n")
    sr_file_name=".\\lab2\\lab2.txt"
    target_file_name=".\\lab2\\lab2.dyd"
    word_list = lex.get_list(sr_file_name)
    print("{} has {} words".format(file_name, len(word_list)))
    lex.lex_main(word_list)
    # print(target)
    with open(target_file_name, "w") as target_file:
        target_file.write(lex.target)
    print("write target in {} sucess".format(target_file_name))

    """
    with PyCallGraph(output=GraphvizOutput()):
        init_files()
        main_procedure()
        end_files()
    """
    init_files()
    main_procedure()
    end_files()