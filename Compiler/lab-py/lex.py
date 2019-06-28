# global temp
temp = ""
state = 0
ItemDict = {
    "begin": 1,
    "end": 2,
    "integer": 3,
    "if": 4,
    "then": 5,
    "else": 6,
    "function": 7,
    "read": 8,
    "write": 9,
    "=": 12,
    "<>": 13,
    "<=": 14,
    "<": 15,
    ">=": 16,
    ">": 17,
    "-": 18,
    "*": 19,
    ":=": 20,
    "(": 21,
    ")": 22,
    ";": 23,
}
line = 0
Token = 10
Constants = 11
target = ""
error = ""
file_name = ".\\lab1\\src.txt"
error_file_name = file_name.split(".txt")[0] + ".err"
print(error_file_name)
target_file_name = file_name.split(".txt")[0] + ".dyd"


def Print(print_word, num):
    global target
    target += print_word.rjust(16) + " " + str(num) + "\n"


def error_write(type):
    global line, error
    pre = "   LINE:"
    if type == 0:
        tem = pre + str(line) + "  出现了无法识别的字符\n"
        error += tem
    elif type == 1:
        tem = pre + str(line) + "  请注意，:后必须跟=\n"
        error += tem
    elif type == 2:
        tem = pre + str(line) + "  错误的关键字\n"
        error += tem
    error_file = open(error_file_name, encoding="utf-8", mode="w")
    error_file.write(error)
    error_file.close()


def end_word():
    global temp
    if temp != "":
        if state == 1:
            if ItemDict.get(temp):
                Print(temp, ItemDict.get(temp))
            else:
                Print(temp, Token)
        elif state == 3:
            Print(temp, Constants)
        else:
            if ItemDict.get(temp):
                Print(temp, ItemDict.get(temp))
            else:
                error_write(1)
        temp = ""


#%%
def get_list(file_name):
    string = []
    with open(file_name) as file:
        for line in file:
            for word in line:
                string.append(word)
    return string


def get_word(string):
    for word in string:
        yield (word)


def is_sym_word():
    global temp
    if ItemDict.get(temp):
        Print(temp, ItemDict.get(temp))
    temp = ""


def lex_main(word_list):
    global state, temp, error, line
    for word in get_word(word_list):
        if word == " ":
            if state != 0:
                end_word()
                state = 0
        # 字母
        elif "z" >= word >= "A":
            if state != 0 and state != 1:
                end_word()
            temp += word
            state = 1
        elif "9" >= word >= "0":
            if state != 3:
                end_word()
            temp += word
            state = 3
        elif word == "=":
            if state not in [0, 10, 14, 17]:
                end_word()
                temp += word
                state = 5
            else:
                temp += word
                end_word()
                state = 0
        elif word == "-":
            end_word()
            temp += word
            state = 6
        elif word == "*":
            end_word()
            temp += word
            state = 7
        elif word == "(":
            end_word()
            temp += word
            state = 8
        elif word == ")":
            end_word()
            temp += word
            state = 9
        elif word == "<":
            end_word()
            temp += word
            state = 10
        elif word == ">":
            if state != 10:
                end_word()
                state = 14
            else:
                temp += word
                state = 12
        elif word == ":":
            end_word()
            temp += word
            state = 17
        elif word == ";":
            end_word()
            temp += word
            state = 20
        elif word == "\n":
            end_word()
            state = 0
            Print("EOLN", 24)
            line += 1
        else:
            error_write(0)
    end_word()
    Print("EOF", 25)


if __name__ == "__main__":
    word_list = get_list(file_name)
    print("{} has {} words".format(file_name, len(word_list)))
    lex_main(word_list)
    # print(target)
    with open(target_file_name, "w") as target_file:
        target_file.write(target)
    print("write target in {} sucess".format(target_file_name))
