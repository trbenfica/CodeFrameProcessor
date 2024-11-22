
def first_sum(x, y):
    def second_sum(x, y):
      def sum(x, y):
         return x + y
      return sum(x, y)
    return second_sum(x, y)
    
first_sum(10, 6)
