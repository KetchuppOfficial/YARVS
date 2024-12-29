import yaml
import argparse
from pathlib import Path


def generate_enum(data : dict[str, dict], output_path : str) -> None:

    enum_values : list[str] = [f"k{id.upper()}" for id in data.keys()]
    enum_values.append("kEndID")
    enum : str = ",\n    ".join(enum_values)

    switch_values : list[str] = [" " *  8 + f"case k{id.upper()}:\n" + \
                                 " " * 12 + f"return \"{id.upper()}\";" for id in data.keys()]
    switch : str = "\n".join(switch_values)

    content : str = f"""/*
 * This file is automatically generated. Do not change it
 */

#ifndef IDENTIFIERS_HPP
#define IDENTIFIERS_HPP

#include <utility>

namespace yarvs
{{

enum InstrID
{{
    {enum}
}};

inline const char *enum_to_str(InstrID id) noexcept
{{
    switch (id)
    {{
{switch}
        default:
            std::unreachable();
    }}
}}

}} // namespace yarvs

#endif // IDENTIFIERS_HPP
"""

    with open(output_path, "w") as enum_file:
        enum_file.write(content)


def generate_single_mask_cases(single_mask_opcodes : list[tuple[int, int]]) -> str:
    cases : list[str] = []

    for opcode, mask in single_mask_opcodes:
        case = " " *  8 + f"case {opcode:#x}:" + "\n" + \
               " " * 12 + f"if (auto decoder = get_decoder(raw_instr & {mask:#x})) [[likely]]\n" + \
               " " * 16 + "return decoder(raw_instr);\n" + \
               " " * 12 + "break;"
        cases.append(case)

    return "\n".join(cases)


def generate_multiple_mask_cases(multiple_mask_opcodes : list[tuple[int, list[int]]]) -> str:
    cases : list[str] = []

    for opcode, masks in multiple_mask_opcodes:
        masks : list[str] = [f"mask_type{{{mask:#x}}}" for mask in sorted(masks, reverse=True)]
        case : str = " " *  8 + f"case {opcode:#x}:\n" + \
                     " " * 12 + f"for (auto mask : {{{", ".join(masks)}}})\n" + \
                     " " * 12 + "{\n" + \
                     " " * 16 + "if (auto decoder = get_decoder(raw_instr & mask))\n" + \
                     " " * 20 + "return decoder(raw_instr);\n" + \
                     " " * 12 + "}\n" + \
                     " " * 12 + "break;"
        cases.append(case)

    return "\n".join(cases)


def generate_decoding_method(data : dict[str, dict]) -> str:
    OPCODE_LEN : int = 7
    INSTR_BIT_LEN : int = 32

    opcode_to_masks : dict[int, list[int]] = dict()
    for info in data.values():
        opcode : int = int(info["encoding"][INSTR_BIT_LEN - OPCODE_LEN : INSTR_BIT_LEN], 2)
        mask : int = int(info["mask"], 16)
        if opcode_to_masks.get(opcode):
            if mask not in opcode_to_masks[opcode]:
                opcode_to_masks[opcode].append(mask)
        else:
            opcode_to_masks[opcode] = [mask]

    singe_mask_opcodes : list[tuple[int, int]] = \
        [(opcode, masks[0]) for opcode, masks in opcode_to_masks.items() if len(masks) == 1]
    multiple_mask_opcodes : list[tuple[int, list[int]]] = \
        list(filter(lambda pair : len(pair[1]) > 1, opcode_to_masks.items()))

    return f"""Instruction Decoder::decode(RawInstruction raw_instr)
{{
    switch (auto opcode = get_bits<6, 0>(raw_instr))
    {{
{generate_single_mask_cases(singe_mask_opcodes)}
{generate_multiple_mask_cases(multiple_mask_opcodes)}
        default: [[unlikely]]
            throw std::invalid_argument{{
                std::format("unknown opcode {{:#x}} of instruction {{:#x}}", opcode, raw_instr)}};
    }}

    throw std::invalid_argument{{std::format("unknown instruction: {{:#x}}", raw_instr)}};
}}"""


def generate_one_instr_case(id : str, info : dict) -> str:
    vars : list[str] = info["variable_fields"]

    out : str = " " * 8  + f"case {info["match"]}:\n" + \
                " " * 12 + "return [](RawInstruction raw_instr) noexcept {\n" + \
                " " * 16 + "return Instruction{\n" + \
                " " * 20 + f".raw = raw_instr,\n" + \
                " " * 20 + f".id = InstrID::k{id.upper()}"

    if any(op in vars for op in ["rs1", "zimm"]):
        out += ",\n" + " " * 20 + ".rs1 = get_bits_r<19, 15, Byte>(raw_instr)"

    if "rs2" in vars:
        out += ",\n" + " " * 20 + ".rs2 = get_bits_r<24, 20, Byte>(raw_instr)"

    if "rd" in vars:
        out += ",\n" + " " * 20 + ".rd = get_bits_r<11, 7, Byte>(raw_instr)"

    if "csr" in vars or any(imm_type in vars for imm_type in ["imm12", "shamtd", "shamtw"]):
        out += ",\n" + " " * 20 + ".imm = decode_i_imm(raw_instr)"
    elif all(imm_type in vars for imm_type in ["imm12hi", "imm12lo"]):
        out += ",\n" + " " * 20 + ".imm = decode_s_imm(raw_instr)"
    elif all(imm_type in vars for imm_type in ["bimm12hi", "bimm12lo"]):
        out += ",\n" + " " * 20 + ".imm = decode_b_imm(raw_instr)"
    elif "imm20" in vars:
        out += ",\n" + " " * 20 + ".imm = decode_u_imm(raw_instr)"
    elif "jimm20" in vars:
        out += ",\n" + " " * 20 + ".imm = decode_j_imm(raw_instr)"
    elif all(field in vars for field in ["fm", "pred", "succ"]): # fence instruction
        out += ",\n" + " " * 20 + ".imm = get_bits<31, 20>(raw_instr)"

    out += "\n" + " " * 16 + "};\n" + " " * 12 + "};"

    return out


def generate_instr_switch(data : dict[str, dict]) -> str:

    cases = [generate_one_instr_case(id, info) for id, info in data.items()]

    return f"""Decoder::decoding_func_type Decoder::get_decoder(match_type match) noexcept
{{
    switch (match)
    {{
{"\n".join(cases)}
        default: [[unlikely]]
            return nullptr;
    }}
}}"""


def generate_decoder(data : dict[str, dict], output_path : str) -> None:
    content : str = f"""/*
 * This file is automatically generated. Do not change it
 */

#include <stdexcept>
#include <format>

#include "bits_manipulation.hpp"
#include "decoder.hpp"
#include "identifiers.hpp"

namespace yarvs
{{

{generate_decoding_method(data)}

{generate_instr_switch(data)}

}} // namespace yarvs
"""

    with open(output_path, "w") as decoder_file:
        decoder_file.write(content)


def generate_executor_declarations(data: dict[str, dict]) -> str:
    decl_list = [" " * 4 + f"static bool exec_{id}(Hart &h, const Instruction &instr);" for id in data.keys()]
    return "\n".join(decl_list)


def generate_exec_table(data : dict[str, dict]) -> str:
    decl_list = [" " * 8 + f"exec_{id}" for id in data.keys()]
    return ",\n".join(decl_list)


def generate_executor_header(data : dict[str, dict], output_path : str) -> None:
    content : str = f"""{generate_executor_declarations(data)}

    static constexpr std::array<callback_type, InstrID::kEndID> kCallbacks_ = {{
{generate_exec_table(data)}
    }};
"""

    with open(output_path, "w") as executor_file:
        executor_file.write(content)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Script for generating C++ code for RISC-V decoder"
    )

    parser.add_argument(
        "path",
        help="Path to YAML file generated by riscv-opcodes"
    )
    parser.add_argument(
        "--enum",
        help="Path to generated C++ header with enum containing IDs of instructions"
    )
    parser.add_argument(
        "--decoder",
        help="Path to generated C++ file with implementation of the decoder"
    )
    parser.add_argument(
        "--exec",
        help="Path to generated C++ header with declarations of executors of instructions"
    )

    args = parser.parse_args()

    if not Path.exists(Path(args.path)):
        raise Exception(f"\"{args.path}\" does not exist")
    if not Path.is_file(Path(args.path)):
        raise Exception(f"\"{args.path}\" is not a file")

    if all(arg == None for arg in [args.enum, args.decoder, args.exec]):
        raise Exception(f"At least one option --enum, --decoder or --exec shall be specified")

    with open(args.path, "r") as yaml_file:
        data = yaml.safe_load(yaml_file)

    if args.enum:
        generate_enum(data, args.enum)

    if args.decoder:
        generate_decoder(data, args.decoder)

    if args.exec:
        generate_executor_header(data, args.exec)


if "__main__" == __name__:
    try:
        main()
    except Exception as e:
        print(f"Caught an instance of type {type(e)}.\nwhat(): {e}")
