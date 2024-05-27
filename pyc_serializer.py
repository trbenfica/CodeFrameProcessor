import binascii
import dis
import marshal
import struct
import sys
import time
import types
import json
import compileall

serialize = False
print_instructions = False
decode_instructions = False
output_file = 'output.json'
input_file = 'input.py'

def initialize_parser(fname):
    f = open(fname, "rb")
    magic = f.read(4)
    read_date_and_size = True
    if sys.version_info >= (3, 7):
        flags = struct.unpack('<L', f.read(4))[0]
        hash_based = bool(flags & 0x01)
        check_source = bool(flags & 0x02)
        if hash_based:
            source_hash = f.read(8)
            read_date_and_size = False
            print(f"hash {binascii.hexlify(source_hash)}")
            print(f"check_source {check_source}")
    if read_date_and_size:
        moddate = f.read(4)
        modtime = time.asctime(time.localtime(struct.unpack('<L', moddate)[0]))
        size = f.read(4)
    code = marshal.load(f)
    if serialize:
        serialized_co = create_dict(code)
        with open(output_file, 'w') as json_file:
            json.dump(serialized_co, json_file, indent=4)
        print('Objeto-código serializado com sucesso!')
    if print_instructions:
        show_code(code)


def show_code(code, indent=''):
    print(f"{indent}{code.co_name!r}:")
    indent += "    "
    show_hex("- co_code:", code.co_code, indent=indent)
    if decode_instructions:
        dis.disassemble(code)
    print("%s- co_consts:" % indent)
    for i, const in enumerate(code.co_consts):
        if type(const) == types.CodeType:
            show_code(const, indent+"    ")
        else:
            print("    %s%d: %r" % (indent, i, const))
    print(f"{indent}- co_names: {code.co_names!r}")
    print(f"{indent}- co_varnames: {code.co_varnames!r}")
    print(f"{indent}- co_freevars: {code.co_freevars!r}")
    print(f"{indent}- co_cellvars: {code.co_cellvars!r}")


def show_hex(label, h, indent):
    h = binascii.hexlify(h)
    print("{}{} {}".format(indent, label, h.decode('ascii')))


def create_dict(code):
    current_CO = dict()
    # current_CO['co_code'] = code.co_code.decode('latin-1')
    current_CO['co_code'] = code.co_code.hex()
    current_CO['co_names'] = code.co_names
    current_CO['co_varnames'] = code.co_varnames
    current_CO['co_freevars'] = code.co_freevars
    current_CO['co_cellvars'] = code.co_cellvars
    current_CO['co_consts'] = list()
    for const in code.co_consts:
        if type(const) == types.CodeType:
            current_CO['co_consts'].append(create_dict(const))
        else:
            current_CO['co_consts'].append(const)
    return current_CO


def print_help():
    help_str = ''' Uso: python pyc_serializer.py [OPÇÕES] input_file.py

    -p \t\t Imprime os bytecodes na tela
    -d \t\t Decodifica os bytecodes, detalhando opcodes e parâmetros. Deve ser usado com -p
    -s \t\t Serializa os objetos-código e os escreve no arquivo especificado por -o'''
    print(help_str)


def main(args):
    global serialize
    global print_instructions
    global decode_instructions
    global output_file
    global input_file

    if args[0].find('h') != -1:
        print_help()
        return
    if len(args) != 2:
        print('Parâmetros incorretos. Use -h para ajuda')
        return
    
    if args[1].find('.py') == -1:
        print('Informe o arquivo .py a ser processado')
        return
    else:
        input_file = args[1]
        if args[0].find('p') != -1:
            print_instructions = True
            if args[0].find('d') != -1:
                decode_instructions = True
        if args[0].find('s') != -1:
            serialize = True
        compileall.compile_file(input_file)
        initialize_parser(f'./__pycache__/{input_file[:-3]}.cpython-310.pyc')
            

if __name__ == '__main__':
    main(sys.argv[1:])
    
