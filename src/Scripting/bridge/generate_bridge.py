#!/usr/bin/env python3
"""Single source of truth for scripting-bridge component reflection.

Reads ComponentSchema.json and generates:
  --cpp-ids    ReflectionIds.gen.hpp      (ReflectedComponentId enum, C++)
  --cpp-tables ReflectionTables.gen.inc   (FieldDesc/ComponentDesc tables + Ensure helper,
                                           included inside the anonymous namespace of
                                           ScriptingBinder.cpp AFTER the accessor templates)
  --csharp     ReflectionSchema.gen.cs    (ReflectedComponentId / FieldType / ComponentFieldInfo, C#)

Schema format:
  fieldTypes: [{name, size}]            transport payload types, mirrored to both languages.
  components: [{name, cpp, dirty, fields}]
    name   - stable component name, also the enum entry (id = index in the list, append-only).
    cpp    - C++ component struct name (re::runtime scope in ScriptingBinder.cpp).
    dirty  - true: render-tracked component (DirtyTag add/remove + MakeDirty on write).
    fields - [{name, kind, member?}]
      kind   - one of KINDS below; "lightType" is a hand-written codec
               (GetLightTypeField/SetLightTypeField), all other kinds map to
               Get/Set<Field>Field<Cpp, &Cpp::member> accessor templates.
      member - C++ member name (required unless the kind is a codec).

Only stdlib is used. Output is deterministic (no timestamps); files are
rewritten only when the content actually changed.
"""

import argparse
import json
import re
import sys
from pathlib import Path
from typing import NoReturn

IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")

# kind -> {ft: FieldType entry, size: payload bytes, get/set: accessor template or codec}.
KINDS = {
    "bool": {"ft": "Bool", "size": 4, "get": "GetBoolField", "set": "SetBoolField"},
    "int32": {"ft": "Int32", "size": 4, "get": "GetIntField", "set": "SetIntField"},
    "float": {"ft": "Float", "size": 4, "get": "GetFloatField", "set": "SetFloatField"},
    "float2": {"ft": "Float2", "size": 8, "get": "GetFloat2Field", "set": "SetFloat2Field"},
    "float3": {"ft": "Float3", "size": 12, "get": "GetFloat3Field", "set": "SetFloat3Field"},
    "color": {"ft": "Color", "size": 4, "get": "GetColorField", "set": "SetColorField"},
    "lightType": {"ft": "Int32", "size": 4, "get": "GetLightTypeField", "set": "SetLightTypeField", "codec": True},
}


def fail(message: str) -> NoReturn:
    print(f"generate_bridge: error: {message}", file=sys.stderr)
    sys.exit(1)


def load_schema(path: Path) -> dict:
    try:
        with open(path, "r", encoding="utf-8-sig") as f:
            schema = json.load(f)
    except (OSError, json.JSONDecodeError) as ex:
        fail(f"cannot read schema '{path}': {ex}")
    if not isinstance(schema, dict):
        fail("schema root must be an object")
    return schema


def validate(schema: dict) -> tuple[list[dict], list[dict]]:
    field_types = schema.get("fieldTypes")
    components = schema.get("components")
    if not isinstance(field_types, list) or not field_types:
        fail("schema needs a non-empty 'fieldTypes' list")
    if not isinstance(components, list) or not components:
        fail("schema needs a non-empty 'components' list")

    known_ft = {}
    for entry in field_types:
        name, size = entry.get("name"), entry.get("size")
        if not isinstance(name, str) or not IDENT.match(name):
            fail(f"bad field type name: {name!r}")
        if not isinstance(size, int) or size <= 0:
            fail(f"bad payload size for field type '{name}'")
        if name in known_ft:
            fail(f"duplicate field type '{name}'")
        known_ft[name] = size

    for kind, spec in KINDS.items():
        if spec["ft"] not in known_ft:
            fail(f"kind '{kind}' maps to unknown field type '{spec['ft']}'")
        if known_ft[spec["ft"]] != spec["size"]:
            fail(f"payload size mismatch for kind '{kind}'")

    seen_names = set()
    for index, comp in enumerate(components):
        for key in ("name", "cpp", "fields"):
            if key not in comp:
                fail(f"component #{index} misses '{key}'")
        name, cpp, fields = comp["name"], comp["cpp"], comp["fields"]
        for token in (name, cpp):
            if not isinstance(token, str) or not IDENT.match(token):
                fail(f"bad identifier: {token!r}")
        if name in seen_names:
            fail(f"duplicate component '{name}'")
        seen_names.add(name)
        if not isinstance(fields, list) or not fields:
            fail(f"component '{name}' needs a non-empty 'fields' list")
        seen_fields = set()
        for field in fields:
            fname, kind = field.get("name"), field.get("kind")
            if not isinstance(fname, str) or not IDENT.match(fname):
                fail(f"bad field name {fname!r} in '{name}'")
            if fname in seen_fields:
                fail(f"duplicate field '{fname}' in '{name}'")
            seen_fields.add(fname)
            if kind not in KINDS:
                fail(f"unknown kind '{kind}' for field '{name}.{fname}'")
            member = field.get("member")
            codec = field.get("codec")
            if codec is not None:
                if not isinstance(codec, dict) or not isinstance(codec.get("get"), str) or not IDENT.match(codec["get"]):
                    fail(f"codec field '{name}.{fname}' needs {{'get': FuncName, 'set': FuncName}}")
                if not isinstance(codec.get("set"), str) or not IDENT.match(codec["set"]):
                    fail(f"codec field '{name}.{fname}' needs {{'get': FuncName, 'set': FuncName}}")
                if member is not None:
                    fail(f"codec field '{name}.{fname}' must not have 'member'")
            elif "codec" in KINDS[kind]:
                if member is not None:
                    fail(f"codec field '{name}.{fname}' must not have 'member'")
            elif not isinstance(member, str) or not IDENT.match(member):
                fail(f"field '{name}.{fname}' needs a valid 'member'")

    return field_types, components


def gen_cpp_ids(field_types: list[dict], components: list[dict]) -> str:
    lines = [
        "// AUTO-GENERATED by generate_bridge.py from ComponentSchema.json - do not edit.",
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "namespace re::scripting",
        "{",
        "",
    ]
    lines.append("enum class FieldType : std::int32_t")
    lines.append("{")
    for i, entry in enumerate(field_types):
        lines.append(f"\t{entry['name']} = {i},")
    lines.append("};")
    lines.append("")
    lines.append("// Component ids are stable: id = index in ComponentSchema.json, append-only.")
    lines.append("enum class ReflectedComponentId : std::int32_t")
    lines.append("{")
    for i, comp in enumerate(components):
        lines.append(f"\t{comp['name']} = {i},")
    lines.append(f"\tCount = {len(components)},")
    lines.append("};")
    lines.append("")
    lines.append("} // namespace re::scripting")
    lines.append("")
    return "\n".join(lines)


def field_row(comp_cpp: str, field: dict) -> str:
    kind = KINDS[field["kind"]]
    ft = f"scripting::FieldType::{kind['ft']}"
    if field.get("codec") is not None:
        codec = field["codec"]
        return f'\t\t{{ "{field["name"]}", {ft}, {kind["size"]}, &{codec["get"]}, &{codec["set"]} }},'
    if "codec" in kind:
        return f'\t\t{{ "{field["name"]}", {ft}, {kind["size"]}, &{kind["get"]}, &{kind["set"]} }},'
    member = field["member"]
    get = f'&{kind["get"]}<{comp_cpp}, &{comp_cpp}::{member}>'
    setter = f'&{kind["set"]}<{comp_cpp}, &{comp_cpp}::{member}>'
    return f'\t\t{{ "{field["name"]}", {ft}, {kind["size"]}, {get}, {setter} }},'


def gen_cpp_tables(components: list[dict]) -> str:
    lines = [
        "// AUTO-GENERATED by generate_bridge.py from ComponentSchema.json - do not edit.",
        "// Included inside the anonymous namespace of ScriptingBinder.cpp,",
        "// AFTER the FieldDesc/ComponentDesc types and accessor templates.",
        "",
    ]
    for comp in components:
        lines.append(f"\t// {comp['name']} ({comp['cpp']})")
        lines.append(f"\tstatic const FieldDesc k{comp['name']}Fields[] = {{")
        for field in comp["fields"]:
            lines.append(field_row(comp["cpp"], field))
        lines.append("\t};")
        lines.append("")
    lines.append("\t// Order must match ReflectedComponentId.")
    lines.append("\tstatic const ComponentDesc kReflectedComponents[] = {")
    for comp in components:
        if comp.get("dirty"):
            lines.append(
                f"\t\t{{ &HasComp<{comp['cpp']}>, &AddComp<{comp['cpp']}>, "
                f"&RemoveRenderComp<{comp['cpp']}>, &MarkDirtyComp<{comp['cpp']}>, "
                f"k{comp['name']}Fields, {len(comp['fields'])} }},"
            )
        else:
            lines.append(
                f"\t\t{{ &HasComp<{comp['cpp']}>, &AddComp<{comp['cpp']}>, "
                f"&RemoveComp<{comp['cpp']}>, nullptr, "
                f"k{comp['name']}Fields, {len(comp['fields'])} }},"
            )
    lines.append("\t};")
    lines.append("")
    lines.append("\tinline constexpr std::int32_t kReflectedComponentCount = "
                f"{len(components)};")
    lines.append("")
    lines.append("\tinline bool RegisterReflectedComponents(ecs::Scene* scene)")
    lines.append("\t{")
    for comp in components:
        lines.append(f"\t\tEnsureComponentRegistered<{comp['cpp']}>(scene);")
    lines.append("\t\treturn true;")
    lines.append("\t}")
    lines.append("")
    return "\n".join(lines)


def gen_csharp(field_types: list[dict], components: list[dict]) -> str:
    lines = [
        "// AUTO-GENERATED by generate_bridge.py from ComponentSchema.json - do not edit.",
        "using System.Runtime.InteropServices;",
        "",
        "namespace EngineAPI",
        "{",
        "    public enum FieldType : int",
        "    {",
    ]
    for i, entry in enumerate(field_types):
        lines.append(f"        {entry['name']} = {i},")
    lines.append("    }")
    lines.append("")
    lines.append("    // Component ids are stable: id = index in ComponentSchema.json, append-only.")
    lines.append("    public enum ReflectedComponentId : int")
    lines.append("    {")
    for i, comp in enumerate(components):
        lines.append(f"        {comp['name']} = {i},")
    lines.append("    }")
    lines.append("")
    lines.append("    public unsafe struct ComponentFieldInfo")
    lines.append("    {")
    lines.append("        public fixed byte Name[48];")
    lines.append("        public FieldType Type;")
    lines.append("        public uint Size;")
    lines.append("    }")
    lines.append("}")
    lines.append("")
    return "\n".join(lines)


def write_if_changed(path: Path, content: str) -> None:
    if path.exists():
        try:
            if path.read_text(encoding="utf-8") == content:
                return
        except OSError as ex:
            fail(f"cannot read '{path}': {ex}")
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")
    except OSError as ex:
        fail(f"cannot write '{path}': {ex}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate scripting bridge reflection tables.")
    parser.add_argument("--schema", required=True)
    parser.add_argument("--cpp-ids", required=True)
    parser.add_argument("--cpp-tables", required=True)
    parser.add_argument("--csharp", required=True)
    args = parser.parse_args()

    schema = load_schema(Path(args.schema))
    field_types, components = validate(schema)

    write_if_changed(Path(args.cpp_ids), gen_cpp_ids(field_types, components))
    write_if_changed(Path(args.cpp_tables), gen_cpp_tables(components))
    write_if_changed(Path(args.csharp), gen_csharp(field_types, components))
    print(f"generate_bridge: {len(components)} components, "
          f"{sum(len(c['fields']) for c in components)} fields")


if __name__ == "__main__":
    main()
