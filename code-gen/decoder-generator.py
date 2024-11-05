import yaml
import argparse
from pathlib import Path


def generate_enum(data : dict[str, dict], output_path : str) -> None:

    enum_values : list[str] = [f"k{id.upper()}" for id in data.keys()]
    enum_values.append("kEndID")
    enum : str = ",\n    ".join(enum_values)

    content : str = f"""
/*
 * This file is automatically generated. Do not change it
 */

#ifndef IDENTIFIERS_HPP
#define IDENTIFIERS_HPP

namespace yarvs
{{

enum InstrID
{{
    {enum}
}};

}} // namespace yarvs

#endif // IDENTIFIERS_HPP
"""

    with open(output_path, "w") as enum_file:
        enum_file.write(content)


def get_mask(opcodes : list[int], masks : list[int], opcode : int) -> int:
    assert len(opcodes) == len(masks)
    if opcode in opcodes:
        return masks[opcodes.index(opcode)]
    else:
        return 0


def generate_masks_array(data : dict[str, dict]) -> str:
    N_OPCODES : int = 2**7
    N_BITS : int = 2**5
    OPCODE_LEN : int = 7

    opcode_to_mask : dict[int, int] = dict()
    for _, info in data.items():
        opcode : int = int(info["encoding"][N_BITS - OPCODE_LEN : N_BITS], 2)
        opcode_to_mask[opcode] = int(info["mask"], 16)

    opcodes, masks = zip(*sorted(opcode_to_mask.items(), key=lambda item : item[0]))
    opcodes : list[int] = list(opcodes)
    masks : list[int] = list(masks)

    lines : list[str] = []
    N_LINES : int = 16
    N_COLS : int = N_OPCODES // N_LINES

    for i in range(N_LINES):
        line : list[str] = [f"{get_mask(opcodes, masks, i * N_COLS):#x}"] + \
                           [f"{get_mask(opcodes, masks, i * N_COLS + j):#10x}" for j in range(1, 8)]
        lines.append(" " * 4 + ", ".join(line))

    return "const std::array<Decoder::mask_type, 128> Decoder::masks_ = {\n" + \
           ",\n".join(lines) + "\n};"


def generate_one_instr(id : str, info : dict) -> str:
    vars : list[str] = info["variable_fields"]

    out : str = " " * 4  + "{" + f"{info["match"]}, [](RawInstruction raw_instr)" + "{\n" + \
                " " * 8  + "return Instruction{\n" + \
                " " * 12 + f".id = InstrID::k{id.upper()},\n"
    if "rs1" in vars:
        out += " " * 12 + ".rs1 = get_bits_r<19, 15, Byte>(raw_instr),\n"
    if "rs2" in vars:
        out += " " * 12 + ".rs2 = get_bits_r<24, 20, Byte>(raw_instr),\n"
    if "rd" in vars:
        out += " " * 12 + ".rd = get_bits_r<11, 7, Byte>(raw_instr),\n"

    if "imm12" in vars: # i-immediate
        out += " " * 12 + ".imm = sext<12, DoubleWord>(get_bits<31, 25>(raw_instr)),\n"
    elif "imm12hi" in vars and "imm12lo" in vars: # s-immediate
        out += " " * 12 + ".imm = decode_s_imm(raw_instr),\n"
    elif "bimm12hi" in vars and "bimm12lo" in vars: # b-immediate
        out += " " * 12 + ".imm = decode_b_imm(raw_instr),\n"
    elif "imm20" in vars: # u-immediate
        out += " " * 12 + ".imm = mask_bits<31, 12>(raw_instr),\n"
    elif "jimm20" in vars: # j-immediate
        out += " " * 12 + ".imm = decode_j_imm(raw_instr),\n"

    out += " " * 12 + f".callback = exec_{id}\n" + " " * 8 + "};\n" + \
           " " * 4 + "}}"

    return out


def generate_map(data : dict) -> str:
    instructions : list = [generate_one_instr(id, info) for id, info in data.items()]
    return "const std::unordered_map<Decoder::match_type, "\
                                    "Decoder::decoding_func_type> Decoder::match_map_ = {\n" + \
           ",\n".join(instructions) + "\n};"

def generate_decoder(data : dict[str, dict], output_path : str) -> None:
    content : str = f"""
/*
 * This file is automatically generated. Do not change it
 */

#include "bits_manipulation.hpp"
#include "decoder.hpp"

#include "executor.hpp"
#include "identifiers.hpp"

namespace yarvs
{{

{generate_masks_array(data)}

{generate_map(data)}

}} // namespace yarvs
"""

    with open(output_path, "w") as decoder_file:
        decoder_file.write(content)


def generate_exec_table(data : dict[str, dict]) -> str:
    decl_list = [f"    exec_{id}" for id in data.keys()]
    return ",\n".join(decl_list)


def generate_executor_header(data : dict[str, dict], output_path : str) -> None:
    content : str = f"""
/*
 * This file is automatically generated. Do not change it.
 * It is intended to be included at the end of executor.cpp.
 * That's why no headers are included here
 */

#ifndef INCLUDE_EXECUTOR_TABLE_HPP
#define INCLUDE_EXECUTOR_TABLE_HPP

namespace yarvs
{{

const std::array<Executor::callback_type, InstrID::kEndID> Executor::callbacks_ = {{
{generate_exec_table(data)}
}};

}} // namespace yarvs

#endif // INCLUDE_EXECUTOR_TABLE_HPP
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
    if args.enum == None and args.decoder == None and args.exec == None:
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
