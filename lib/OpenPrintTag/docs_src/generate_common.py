import shutil
import os
import sys
import jinja2
import jinja2.ext
import jinja2.nodes
import subprocess

from vars import *

# Re-create output directory
shutil.rmtree(out_dir, ignore_errors=True)
os.mkdir(out_dir)

# Copy the docsify index.html
shutil.copyfile(f"{dir}/index.html", f"{out_dir}/index.html")

# Set up jinja
env = jinja2.Environment(loader=jinja2.FileSystemLoader(dir))
args = {}

# Tables support
from tables import *


def region_type_transform(cell, row):
    result = cell
    if "max_length" in row:
        result = f"{result}:{row['max_length']}"

    return f"`{result}`"


cbor_region_columns = [
    Column(field="name", title="Name", transform=lambda x: f"`{x}`"),
    Column(field="key", title="Key"),
    Column(field="type", title="Type", transform_ext=region_type_transform),
    Column(field="unit", title="Unit"),
    Column(field="example", title="Example"),
    Column(field="description", title="Description", transform_ext=desc_transform),
]

enum_columns = [
    Column(field="name", title="Name", transform=lambda x: f"`{x}`"),
    Column(field="key", title="Key"),
    Column(field="description", title="Description"),
]

env.globals["fields_table"] = lambda file, category=None: generate_table(f"{data_dir}/{file}.yaml", cbor_region_columns, lambda row: (category is None) or row.get("category", "") == category)
env.globals["enum_table"] = lambda file, columns=enum_columns: generate_table(f"{data_dir}/{file}.yaml", columns)

# Examples support


class PythonCodeExtension(jinja2.ext.Extension):
    tags = {"python"}

    def parse(self, parser):
        next(parser.stream)
        body = parser.parse_statements(["name:endpython"], drop_needle=True)
        return jinja2.nodes.CallBlock(self.call_method("_render"), [], [], body)

    def _render(self, caller):
        code = caller()
        result = f"> ```python\n> {code.strip().replace('\n', '\n> ')}\n> ```\n"

        out_buf = io.StringIO()
        old_stdout = sys.stdout
        sys.stdout = out_buf
        exec(code, globals().copy())
        sys.stdout = old_stdout

        result += f"```\n{out_buf.getvalue()}```"

        return result


env.add_extension(PythonCodeExtension)


def show_example(prompt, language="yaml"):
    r = io.StringIO("")

    nice_prompt = prompt.replace(">", "").replace(" | ", "\n> | ")
    r.write(f"> ```bash\n>{nice_prompt}\n> ```\n\n")

    prompt = prompt.replace(">", f"python3 {utils_dir}/")
    prompt = prompt.replace("--config-file=", f"--config-file={os.path.abspath(data_dir)}/")
    prompt = "set -o pipefail; " + prompt
    output = subprocess.run(prompt, shell=True, stdout=subprocess.PIPE, check=False, cwd=dir, executable="/bin/bash")

    if output.returncode != 0:
        print(f"Error while running '{prompt}'")
        sys.exit(1)

    r.write("<details><summary><b>Commmand output</b></summary>\n\n")
    r.write(f"```{language}\n{output.stdout.decode()}\n```\n")
    r.write("</details>\n\n")

    return r.getvalue()


env.globals["show_example"] = show_example


def show_file(file, language="yaml"):
    r = io.StringIO("")

    r.write(f"> ```bash\n> cat {file}\n> ```\n\n")

    r.write("<details><summary><b>Commmand output</b></summary>\n\n")
    with open(f"{dir}/{file}", "r", encoding="utf-8") as f:
        r.write(f"```{language}\n{f.read()}\n```\n")
    r.write("</details>\n\n")

    return r.getvalue()


env.globals["show_file"] = show_file


# Other variables
env.globals["repo"] = repo
env.globals["arch"] = arch


# Generate documentation files
def gen_doc_file(source_file):
    with open(f"{out_dir}/{source_file}.md", "w") as f:
        f.write(env.get_template(f"{source_file}.md").render(args))
