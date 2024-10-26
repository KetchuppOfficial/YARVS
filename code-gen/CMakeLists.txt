set(BUILD_CODEGEN_DIR ${PROJECT_BINARY_DIR}/code-gen)
set(BUILD_INCLUDE_DIR ${PROJECT_BINARY_DIR}/include)

file(COPY ${CODEGEN_DIR}/decoder-generator.py DESTINATION ${BUILD_CODEGEN_DIR})
file(COPY ${CODEGEN_DIR}/requirements.txt DESTINATION ${BUILD_CODEGEN_DIR})
file(MAKE_DIRECTORY ${BUILD_INCLUDE_DIR})

set(RISCV_YAML ${PROJECT_SOURCE_DIR}/riscv-opcodes/instr_dict.yaml)

add_custom_command(
    OUTPUT ${RISCV_YAML}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/riscv-opcodes
    COMMAND ${Python_EXECUTABLE} parse.py rv_i rv64_i
    DEPENDS ${PROJECT_SOURCE_DIR}/riscv-opcodes/parse.py
)

set(INSTRUCTION_IDS ${BUILD_INCLUDE_DIR}/identifiers.hpp)

add_custom_command(
    OUTPUT ${INSTRUCTION_IDS}
    WORKING_DIRECTORY ${BUILD_CODEGEN_DIR}
    COMMAND ${Python_EXECUTABLE} -m venv .venv
    COMMAND . .venv/bin/activate &&
            pip3 install -r requirements.txt --require-virtualenv
    COMMAND . .venv/bin/activate &&
            ${Python_EXECUTABLE} ${BUILD_CODEGEN_DIR}/decoder-generator.py ${RISCV_YAML}
                --enum ${INSTRUCTION_IDS}
    DEPENDS ${CODEGEN_DIR}/decoder-generator.py ${RISCV_YAML}
)

add_custom_target(code_generator
    DEPENDS ${INSTRUCTION_IDS}
)
