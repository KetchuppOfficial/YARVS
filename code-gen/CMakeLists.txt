set(BUILD_CODEGEN_DIR ${PROJECT_BINARY_DIR}/code-gen)

configure_file(${CODEGEN_DIR}/generator.py ${BUILD_CODEGEN_DIR} COPYONLY)
configure_file(${CODEGEN_DIR}/requirements.txt ${BUILD_CODEGEN_DIR} COPYONLY)

file(MAKE_DIRECTORY ${BUILD_INCLUDE_DIR})
file(MAKE_DIRECTORY ${BUILD_SOURCE_DIR})

set(RISCV_YAML ${PROJECT_SOURCE_DIR}/riscv-opcodes/instr_dict.yaml)

add_custom_command(
    COMMENT "Generating YAML with description of instructions"
    OUTPUT ${RISCV_YAML}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/riscv-opcodes
    COMMAND ${Python_EXECUTABLE} parse.py rv_i rv64_i rv_zicsr rv_s rv_system
)

add_custom_command(
    COMMENT "Creating virtual environment for code generation"
    OUTPUT ${BUILD_CODEGEN_DIR}/.venv
    WORKING_DIRECTORY ${BUILD_CODEGEN_DIR}
    COMMAND ${Python_EXECUTABLE} -m venv .venv
    COMMAND . .venv/bin/activate &&
            pip3 install -r requirements.txt --require-virtualenv
    DEPENDS ${CODEGEN_DIR}/requirements.txt
)

set(INSTRUCTION_IDS ${BUILD_INCLUDE_DIR}/identifiers.hpp)
set(DECODER ${BUILD_SOURCE_DIR}/decoder.cpp)
set(EXECUTOR_DECLARATIONS ${BUILD_INCLUDE_DIR}/executor_declarations.hpp)

add_custom_command(
    COMMENT "Generating code for decoder and executor"
    OUTPUT ${INSTRUCTION_IDS} ${DECODER} ${EXECUTOR_DECLARATIONS}
    WORKING_DIRECTORY ${BUILD_CODEGEN_DIR}
    COMMAND . .venv/bin/activate &&
            ${Python_EXECUTABLE} ${BUILD_CODEGEN_DIR}/generator.py ${RISCV_YAML}
                --enum ${INSTRUCTION_IDS}
                --decoder ${DECODER}
                --exec ${EXECUTOR_DECLARATIONS}
    DEPENDS ${CODEGEN_DIR}/generator.py ${RISCV_YAML} ${BUILD_CODEGEN_DIR}/.venv
)

add_custom_target(code_generator
    DEPENDS ${INSTRUCTION_IDS} ${DECODER} ${EXECUTOR_DECLARATIONS}
)
