configure_file(generator.py generator.py COPYONLY)
configure_file(requirements.txt requirements.txt COPYONLY)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/yarvs)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src)

set(RISCV_YAML ${PROJECT_SOURCE_DIR}/riscv-opcodes/instr_dict.yaml)

add_custom_command(
    COMMENT "Generating YAML with description of instructions"
    OUTPUT ${RISCV_YAML}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/riscv-opcodes
    COMMAND ${Python_EXECUTABLE} parse.py rv_i rv64_i rv_zicsr rv_s rv_system
)

add_custom_command(
    COMMENT "Creating virtual environment for code generation"
    OUTPUT ./.venv
    COMMAND ${Python_EXECUTABLE} -m venv .venv
    COMMAND .venv/bin/pip3 install -r requirements.txt --require-virtualenv
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
)

set(INSTRUCTION_IDS ./include/yarvs/identifiers.hpp)
set(DECODER ./src/decoder.cpp)
set(EXECUTOR_DECLARATIONS ./include/yarvs/executor_declarations.hpp)
set(INSTR_DUMP_DEFINITION ./src/instruction.cpp)

add_custom_command(
    COMMENT "Generating code for decoder and executor"
    OUTPUT ${INSTRUCTION_IDS} ${DECODER} ${EXECUTOR_DECLARATIONS} ${INSTR_DUMP_DEFINITION}
    COMMAND .venv/bin/python3 generator.py ${RISCV_YAML}
                --enum ${INSTRUCTION_IDS}
                --decoder ${DECODER}
                --exec ${EXECUTOR_DECLARATIONS}
                --dump ${INSTR_DUMP_DEFINITION}
    DEPENDS ${RISCV_YAML} ./.venv ${CMAKE_CURRENT_SOURCE_DIR}/generator.py
)

add_custom_target(code_generator
DEPENDS
    ${INSTRUCTION_IDS}
    ${DECODER}
    ${EXECUTOR_DECLARATIONS}
    ${INSTR_DUMP_DEFINITION}
)
