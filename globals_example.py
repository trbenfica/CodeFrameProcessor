
a = "global"
b = 23
c = True

def join(x):
    if c:
        return x + a + b
    else:
        return -1


print(join('testing'))

