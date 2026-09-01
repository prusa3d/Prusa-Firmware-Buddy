from generate_common import gen_doc_file, env, Column, dir, out_dir
import vars
import shutil
import io
import yaml
import os


def gen_material_tag_table():
    r = io.StringIO("")
    r.write("<table>")
    r.write("<tr><th>ID</th><th>Name</th><th>Display name</th><th>Info</th>")

    tags = yaml.safe_load(open(os.path.join(vars.data_dir, "tags_enum.yaml"), "r", encoding="utf-8"))
    categories = yaml.safe_load(open(os.path.join(vars.data_dir, "tag_categories_enum.yaml"), "r", encoding="utf-8"))

    categories_keys = {c["name"] for c in categories}

    # Check that all tags have a matching category
    for tag in tags:
        if tag.get("deprecated", False):
            continue

        assert tag["category"] in categories_keys, f"Tag {tag['name']} category {tag['category']} is not in material_tag_categories.yaml"

    for cat in categories:
        r.write(f"<tr><th colspan='4' align='left'>{cat['emoji']} {cat['display_name']}</th></tr>")

        for tag in tags:
            if tag.get("deprecated", False):
                continue

            if tag["category"] != cat["name"]:
                continue

            r.write("<tr>")
            r.write(f"<td>{tag['key']}</td>")
            r.write(f"<td><code>{tag['name']}</code></td>")
            r.write(f"<td>{tag['display_name']}</td>")

            r.write("<td>")

            desc_lines = []

            desc = tag.get("description", [])
            if isinstance(desc, list):
                desc_lines += desc
            elif len(desc.strip()) > 0:
                desc_lines.append(desc)

            implies = tag.get("implies", [])
            if len(implies) > 0:
                desc_lines.append("Implies " + ", ".join(map(lambda i: f"<code>{i}</code>", implies)))

            hints = tag.get("hints", [])
            if len(hints) > 0:
                desc_lines.append("Hints " + ", ".join(map(lambda i: f"<code>{i}</code>", hints)))

            r.write("<br>".join(desc_lines))
            r.write("</td></tr>")

    r.write("</table>\n\n")
    r.write(f"*This table was automatically generated from [`tags_enum.yaml`]({vars.repo}/blob/main/data/tags_enum.yaml) and [`tag_categories_enum.yaml`]({vars.repo}/blob/main/data/tag_categories_enum.yaml)*\n\n")

    return r.getvalue()


env.globals["material_tag_table"] = gen_material_tag_table

env.globals["fff_material_type_columns"] = [
    Column(field="key", title="Key"),
    Column(field="abbreviation", title="Name", transform=lambda x: f"`{x}`"),
    Column(field="name", title="Full name"),
    Column(field="description", title="Description"),
]

env.globals["material_certification_columns"] = [
    Column(field="key", title="Key"),
    Column(field="name", title="Name", transform=lambda x: f"`{x}`"),
    Column(field="display_name", title="Display name"),
    Column(field="description", title="Description"),
]

gen_doc_file("_navbar")
gen_doc_file("_sidebar")
gen_doc_file("README")
gen_doc_file("changelog")

gen_doc_file("terminology")
gen_doc_file("nfc_data_format")
gen_doc_file("nfc_technical_details")
gen_doc_file("examples")
gen_doc_file("contributing")
gen_doc_file("fff_material_types")
gen_doc_file("material_tags")
gen_doc_file("material_certifications")
gen_doc_file("physical_spec")

shutil.copytree(f"{dir}/media", f"{out_dir}/media", dirs_exist_ok=True)
