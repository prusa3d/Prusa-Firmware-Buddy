import argparse
import inspect
import os
import sys
import yaml

from pathlib import Path


err_class_mapping = {
    1: "ERR_MECHANICAL",
    2: "ERR_TEMPERATURE",
    3: "ERR_ELECTRO",
    4: "ERR_CONNECT",
    5: "ERR_SYSTEM",
    6: "BOOTLOADER",
    7: "WARNING",
    9: "ERR_OTHER"
}

mmu_template = \
"""#pragma once
#include "inttypes.h"
#include "button_operations.h"

#include <array>

{include_items}

namespace MMU2 {{

inline constexpr uint8_t ERR_MMU_CODE = {printer_code};

enum class ErrType : uint8_t {{
    ERROR = 0,
    WARNING,
    USER_ACTION
}};

enum class ErrCode : uint16_t {{
    ERR_UNDEF = 0,
    {enum_items}
}};

struct MMUErrDesc {{
    // 32 bit
    const char *err_title;
    const char *err_text;
    // 16 bit
    ErrCode err_code;
    std::array<ButtonOperations, 3> buttons;
    ErrType type;
}};

}}  // namespace MMU2
"""

mmu_list_template = \
"""#pragma once
#include "i18n.h"

{include_items}

namespace MMU2 {{

inline constexpr MMUErrDesc error_list[] = {{{list_items}
}};

}}  // namespace MMU2
"""

buddy_template = \
"""#pragma once
#include "inttypes.h"

{include_items}

inline constexpr uint8_t ERR_PRINTER_CODE = {printer_code};

enum class ErrCode : uint16_t {{
    ERR_UNDEF = 0,
    {enum_items}
}};

struct ErrDesc {{
    // 32 bit
    const char *err_title;
    const char *err_text;
    // 16 bit
    ErrCode err_code;
}};
"""

buddy_list_template = \
"""#pragma once
#include "i18n.h"

{include_items}

inline constexpr ErrDesc error_list[] = {{{list_items}
}};
"""

def generate_header_file(yaml_file_name, header_file_name, mmu, list, includes):
    with open(yaml_file_name, "r") as yaml_file:
        parsed_file = yaml.safe_load(yaml_file)

        printer_code = None

        err_id_cache = set()
        err_dict = {}

        for err in parsed_file["Errors"]:
            code = err["code"]
            assert len(code) == 5, f"Error code {code} is not five digits."

            curr_printer_code = int(code[0:2])
            if printer_code is None:
                printer_code = curr_printer_code
            else:
                assert curr_printer_code == printer_code, \
                f"Printer code {curr_printer_code} of error code {code} is not " + \
                f"the same as previously specified printer code {printer_code}."

            err_class = int(code[2:3])
            assert err_class in err_class_mapping, f"Unknown error class {err_class} in error code {code}."

            err_id = f"{err_class_mapping[err_class]}_{err['id']}"
            assert err_id not in err_id_cache, f"Duplicate error id \"{err_id}\"."
            err_id_cache.add(err_id)

            err_code = int(code)
            assert err_code not in err_dict, f"Duplicate error code {code}."

            if mmu:
                btns = [f"ButtonOperations::{action}" for action in err["action"]]

                # Make sure the array always has 3 buttons, NoOperation button
                # occupying the appropriate "empty" slots
                if len(btns) == 0:
                    btns.append("ButtonOperations::NoOperation")
                if len(btns) == 1:
                    btns.insert(0, "ButtonOperations::NoOperation")
                if len(btns) == 2:
                    btns.append("ButtonOperations::NoOperation")

                mmu_extra_text = f",\n        {{{', '.join(btns)}}}"

                if "type" not in err:
                    err["type"] = "ERROR"

                mmu_extra_text += f",\n        ErrType::{err['type']}"
            else:
                mmu_extra_text = ""

            err_dict[err_code] = {
                "id": err_id,
                "code": err_code,
                "title": err["title"],
                "text": err["text"].replace("\n", "\\n"),
                "mmu_extra_text": mmu_extra_text
            }

    os.makedirs(header_file_name.parent, exist_ok=True)

    enum_items = ",\n    ".join(f"{err['id']} = {err['code']}" for err in err_dict.values())

    list_items = ",".join(f"""
    {{
        N_("{err['title']}"),
        N_("{err['text']}"),
        ErrCode::{err['id']}{err['mmu_extra_text']}
    }}""" for err in err_dict.values())

    include_items = "\n".join([f"#include <{item}>" for item in includes])

    if mmu:
        if list:
            template = mmu_list_template
        else:
            template = mmu_template
    else:
        if list:
            template = buddy_list_template
        else:
            template = buddy_template

    content = template.format(printer_code=printer_code, enum_items=enum_items, list_items=list_items, include_items=include_items)

    with open(header_file_name, 'w') as f:
        f.write(content)


def main(args):
    generate_header_file(getattr(args, "yaml-file"),
                         getattr(args, "output-file"),
                         args.mmu, args.list, args.include)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('yaml-file', type=Path)
    parser.add_argument('output-file', type=Path)
    parser.add_argument('--mmu', default=False, action='store_true', help='Generate mmu error codes')
    parser.add_argument('--list', default=False, action='store_true', help='Generate error list, requires translations')
    parser.add_argument('--include', default=[], action='append', help='List of files to include')
    args = parser.parse_args(sys.argv[1:])
    main(args)
