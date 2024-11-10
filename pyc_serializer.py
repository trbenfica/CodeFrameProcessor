import binascii
import dis
import marshal
import struct
import sys
import time
import types
import json
import compileall
import getopt
import os

serialize = False
print_instructions = False
decode_instructions = False
output_file = 'code.json'
input_file = 'input.py'

def initialize_parser(fname):
    with open(fname, "rb") as f:
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
    h = binascii.hexlify(code.co_code)
    print(f"{indent}- co_code: {h.decode('ascii')}")
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

def create_dict(code):
    current_CO = dict()
    current_CO['co_code'] = code.co_code.hex()
    current_CO['co_names'] = code.co_names
    current_CO['co_varnames'] = code.co_varnames
    current_CO['co_freevars'] = code.co_freevars
    current_CO['co_cellvars'] = code.co_cellvars
    current_CO['co_consts'] = []
    for const in code.co_consts:
        if type(const) == types.CodeType:
            current_CO['co_consts'].append(create_dict(const))
        else:
            current_CO['co_consts'].append(const)
    return current_CO

def print_help():
    help_str = '''Uso: python pyc_serializer.py [OPÇÕES] -i input_file.py

    -p          Imprime os bytecodes na tela
    -d          Decodifica os bytecodes, detalhando opcodes e parâmetros. Deve ser usado com -p
    -s          Serializa os objetos-código e os escreve no arquivo especificado por -o
    -o output   Especifica o arquivo de saída para a serialização (default: output.json)
    -h          Mostra esta mensagem de ajuda
    -i          Indica o arquivo de entrada
    '''
    print(help_str)

def main(argv):
    global serialize
    global print_instructions
    global decode_instructions
    global output_file
    global input_file

    try:
        opts, args = getopt.getopt(argv, "hpdso:i:")
    except getopt.GetoptError:
        print_help()
        sys.exit(2)

    for opt, arg in opts:
        if opt == '-h':
            print_help()
            sys.exit()
        elif opt == '-p':
            print_instructions = True
        elif opt == '-d':
            decode_instructions = True
        elif opt == '-s':
            serialize = True
        elif opt == '-o':
            output_file = arg
        elif opt == '-i':
            input_file = arg

    if not input_file.endswith('.py'):
        print('Informe um arquivo .py válido para processar')
        sys.exit(2)

    if print_instructions and not os.path.isfile(input_file):
        print('Arquivo de entrada não encontrado')
        sys.exit(2)

    compileall.compile_file(input_file)
    pyc_file = f'./__pycache__/{os.path.basename(input_file[:-3])}.cpython-{sys.version_info[0]}{sys.version_info[1]}.pyc'
    if os.path.isfile(pyc_file):
        initialize_parser(pyc_file)
    else:
        print(f'Arquivo compilado {pyc_file} não encontrado')

if __name__ == '__main__':
    main(sys.argv[1:])
