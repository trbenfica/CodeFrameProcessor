class Pessoa:
    
    def say_hi(self, name="yanker"):
        print(f'hi {name}')

    def __init__(self, name, age, occupation):
        self.name = name
        self.age = age
        self.occupation = occupation
        Pessoa.populacao += 1

    def to_string(self):
        print(f'Nome: {self.name}, idade: {self.age}, ocupação: {self.occupation}')

    populacao = 0

    @classmethod
    def total_pessoas(cls):
        print(f'Total de pessoas é {Pessoa.populacao}')


joao = Pessoa("Joao", 23, "Computer Engineer")
marcos = Pessoa("marcos", 27, "Computer Engineer")
Carlos = Pessoa("Carlos", 17, "Computer Scientist")
Pessoa.total_pessoas()
