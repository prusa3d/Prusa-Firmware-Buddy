import yaml
import typing
import io
import os

from vars import *


def default_transform(data: any):
    match data:
        case None:
            return ""

        case bool():
            return "yes" if data else "no"

        case list():
            return "<br>".join(str(x) for x in data)

        case _:
            return str(data).replace("\n", "<br>")


def desc_transform(cell: any, row: any):
    match cell:
        case None:
            result = ""

        case list():
            result = "\n".join(str(x) for x in cell)

        case str():
            result = cell

        case _:
            assert False

    match row.get("required"):
        case True:
            result += "\n\n**Required field.**"

        case False | None:
            pass

        case "recommended":
            result += "\n\n**Recommended field.**"

        case _:
            assert False

    return result.strip().replace("\n", "<br>")


class Column(typing.NamedTuple):
    field: str
    title: str
    transform: any = default_transform
    transform_ext: any = None


def generate_table(yaml_file: str, columns: typing.List[Column], filter: any = None):
    src = open(yaml_file, "r", encoding="utf-8")
    data = yaml.safe_load(src)

    tgt = io.StringIO("")
    basename = os.path.basename(yaml_file)

    # Generate table header
    tgt.write("|")
    for col in columns:
        tgt.write(col.title + "|")
    tgt.write("\n")

    tgt.write("|")
    for col in columns:
        tgt.write(":--|")
    tgt.write("\n")

    for row in data:
        if filter and not filter(row):
            continue

        if row.get("deprecated", False):
            continue

        tgt.write("|")
        for col in columns:
            cell = row.get(col.field, None)

            if col.transform_ext:
                cell = col.transform_ext(cell, row)
            else:
                cell = col.transform(cell)

            tgt.write(cell)
            tgt.write("|")
        tgt.write("\n")

    tgt.write("\n")
    tgt.write(f"*This table was automatically generated from [`{basename}`]({repo}/blob/main/data/{basename})*\n\n")

    return tgt.getvalue()
