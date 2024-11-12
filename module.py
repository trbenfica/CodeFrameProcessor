
a = 8
b = 2
c = True

def compute(x, y):
    if c:
        return sum(x, y) + a
    else:
        return sum(x, y) + a + 10

def sum(x, y):
  return x + y + 10

compute(10, 6)



c = a + b
