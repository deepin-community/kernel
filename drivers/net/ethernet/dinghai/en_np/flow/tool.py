import os
import sys
import xml.etree.ElementTree as ET

def parse_flow_attr_xml(xml_file):
    tree = ET.parse(xml_file)
    root = tree.getroot()

    sdt_templates = {}  # 保存sdt-template内容
    structures = []  # 保存最终SDT结构体内容

    # 解析SDT模板(sdt-template)
    for sdt_template in root.findall("sdt-template"):
        sdt_template_id = sdt_template.attrib["id"]
        sdt_type = sdt_template.attrib.get("type", "")
        sdt_name = sdt_template.attrib.get("name", "")
        width = int(sdt_template.attrib.get("width", 0))
        key_width = int(sdt_template.attrib.get("key", 0))
        rst_width = int(sdt_template.attrib.get("rst", 0))
        fields = []

        # 解析sdt-template中的field定义
        for field in sdt_template.findall("field"):
            fields.append({
                "name": field.attrib["name"],
                "attr": field.attrib["attr"],
                "array_num": int(field.attrib["array_num"]),
                "element_size": int(field.attrib["element_bits"]) // 8,
                "msb": int(field.attrib["msb"]),
                "length": int(field.attrib["bit_num"]),
            })

        sdt_templates[sdt_template_id] = {
            "type": sdt_type,
            "name": sdt_name,
            "width": width,
            "key_width": key_width,
            "rst_width": rst_width,
            "fields": fields,
        }

    # 解析具体SDT实例(sdt)
    for sdt in root.findall("sdt"):
        sdt_no = int(sdt.attrib["no"])
        use_template = sdt.attrib.get("use-template")

        if use_template:  # 如果实例引用模板
            if use_template in sdt_templates:  # 引用了sdt-template
                template_data = sdt_templates[use_template]
                sdt_type = template_data["type"]
                sdt_name = template_data["name"]
                width = template_data["width"]
                key_width = template_data["key_width"]
                rst_width = template_data["rst_width"]
                fields = template_data["fields"]
            else:
                raise ValueError(f"Undefined sdt-template reference: {use_template}")
        else:  # 如果没有引用模板，解析自身定义
            sdt_type = sdt.attrib.get("type", "")
            sdt_name = sdt.attrib.get("name", "")
            width = int(sdt.attrib.get("width", 0))
            key_width = int(sdt.attrib.get("key", 0))
            rst_width = int(sdt.attrib.get("rst", 0))
            fields = []
            for field in sdt.findall("field"):
                fields.append({
                    "name": field.attrib["name"],
                    "attr": field.attrib["attr"],
                    "array_num": int(field.attrib["array_num"]),
                    "element_size": int(field.attrib["element_bits"]) // 8,
                    "msb": int(field.attrib["msb"]),
                    "length": int(field.attrib["bit_num"]),
                })

        # 如果类型是 ACL，设置 key_width 和处理 fields
        if sdt_type == "ACL":
            key_width = width  # key_width 等于 width
            additional_fields = []
            for field in fields:
                if field["attr"] == "key":
                    mask_field = field.copy()
                    mask_field["attr"] = "mask"
                    additional_fields.append(mask_field)
            fields.extend(additional_fields)

            # 调整字段顺序为 key -> mask -> rst
            fields.sort(key=lambda x: {"key": 0, "mask": 1, "rst": 2}.get(x["attr"], 3))

        # 将解析后的 SDT 添加到结构体
        structures.append({
            "no": sdt_no,
            "type": sdt_type,
            "name": sdt_name,
            "width": width,
            "key_width": key_width,
            "rst_width": rst_width,
            "fields": fields,
        })

    return structures



def check_parse(structures):
    for structure in structures:
        # 获取当前结构体的字段列表
        fields = structure["fields"]

        # 将字段按 attr 分类
        fields_by_attr = {}
        for field in fields:
            attr = field["attr"]
            if attr not in fields_by_attr:
                fields_by_attr[attr] = []
            fields_by_attr[attr].append(field)

        # 按分类逐一检查字段的连续性和长度
        for attr, fields_list in fields_by_attr.items():
            # 按 msb 从大到小排序
            fields_list.sort(key=lambda f: f["msb"], reverse=True)

            # 检查连续性和 array_num * 8 = length 的关系
            for i in range(len(fields_list)):
                current_field = fields_list[i]

                # 如果不是最后一个字段，检查连续性
                if i < len(fields_list) - 1:
                    next_field = fields_list[i + 1]
                    if current_field["msb"] - current_field["length"] != next_field["msb"]:
                        print(f"Error in Structure No: {structure['no']} for attr: {attr}")
                        print(f"Field {current_field['name']} and {next_field['name']} are not continuous.")
                        return False

    # 如果所有结构体和字段都检查通过
    return True


def print_structures(structures):
    # 遍历 structures 列表
    for structure in structures:
        print(f"Structure No: {structure['no']}")
        print(f"  Type: {structure['type']}")
        print(f"  Name: {structure['name']}")
        print(f"  Width: {structure['width']}")
        print(f"  Key Width: {structure['key_width']}")
        print(f"  Rst Width: {structure['rst_width']}")

        # 打印字段列表
        print("  Fields:")
        for field in structure['fields']:
            print(f"    - {field}")
        print("-" * 30)  # 分隔符


def generate_tbl_c_file(structures):
    # 查看数组内容是否全部正确写入
    # print_structures(structures)

    # 用于记录已生成的结构体名称，避免重复生成
    generated_names = set()

    # 生成字段数组
    for struct in structures:
        if not struct["name"]:
            continue
        if struct["name"] in generated_names:
            continue
        generated_names.add(struct["name"])
        output_file = f"source/dpp_tbl_{struct['name']}.c".lower()

        with open(output_file, "w") as f:
            f.write('#include "dpp_flow_comm.h"\n')
            f.write(f'#include "dpp_tbl_{struct["name"]}.h"\n')
            f.write(f"ZXDH_FLOW_ATTR_FIELD_T g_{struct['name']}_fields[] = \n{{\n")
            for field in struct["fields"]:
                f.write(
                    f'    {{"{field["name"]}", DPP_ATTR_FLAG_{field["attr"].upper()}, '
                    f'{field["array_num"]}, {field["element_size"]}, {field["msb"]}, {field["length"]}}},\n'
                )
            f.write("};\n\n")


def generate_tbl_list_c_file(structures, output_file):
    # 检查目标文件是否存在，存在则删除
    if os.path.exists(output_file):
        os.remove(output_file)

    with open(output_file, "w") as f:
        f.write('#include "dpp_flow_comm.h"\n')

        generated_names0 = set()
        for struct in structures:
            if struct["name"] in generated_names0:
                continue
            generated_names0.add(struct["name"])
            f.write(f'#include "dpp_tbl_{struct["name"].lower()}.h"\n')

        # 查看数组内容是否全部正确写入
        print_structures(structures)
        # 用于记录已生成的结构体名称，避免重复生成
        generated_names = set()

        # 生成字段数组
        for struct in structures:
            if not struct["name"]:
                continue
            if struct["name"] in generated_names:
                continue
            generated_names.add(struct["name"])

            f.write(f"ZXDH_FLOW_ATTR_FIELD_T g_{struct['name']}_fields[] = \n{{\n")
            for field in struct["fields"]:
                f.write(
                    f'    {{"{field["name"]}", DPP_ATTR_FLAG_{field["attr"].upper()}, '
                    f'{field["array_num"]}, {field["element_size"]}, {field["msb"]}, {field["length"]}}},\n'
                )
            f.write("};\n\n")

        # 生成流属性列表
        f.write("ZXDH_FLOW_ATTR_T g_flow_attr_list[]= \n{\n")
        for struct in structures:
            # if not struct["no"]:
            #     continue
            f.write("    {\n")
            f.write(f'        "{struct["name"]}",\n')
            f.write(f"         {struct['no']},\n")
            f.write(f"         DPP_FLOW_SDT_{struct['type'].upper()},\n")
            f.write(f"         {struct['width']},\n")
            f.write(f"         {struct['key_width']},\n")
            f.write(f"         {struct['rst_width']},\n")
            f.write(f"         {len(struct['fields'])},\n")
            f.write(f"         g_{struct['name']}_fields\n")
            f.write("    },\n")
        f.write("};\n")

        f.write("ZXIC_UINT32 dpp_flow_attr_list_size_get(void)\n")
        f.write("{\n")
        f.write("    return sizeof(g_flow_attr_list)/sizeof(ZXDH_FLOW_ATTR_T);\n")
        f.write("}\n")


def generate_tbl_h_file(structures):
    # 用于存储已生成的结构体名称，避免重复生成
    generated_names = set()

    for structure in structures:
        struct_name = f"zxdh_{structure['name']}".lower()
        output_file = f"{sys.argv[1]}/include/dpp_tbl_{structure['name']}.h".lower()

        # 检查是否已经生成该结构体
        if struct_name in generated_names:
            continue
        generated_names.add(struct_name)

        with open(output_file, "w") as f:
            # 写入头文件保护
            f.write(f"#ifndef DPP_TBL_{structure['name'].upper()}_H\n")
            f.write(f"#define DPP_TBL_{structure['name'].upper()}_H\n\n")
            f.write('#include "zxic_common.h"\n\n')

            if structure["type"] == "ERAM" or structure["type"] == "DDR":
                # ERAM 和 DDR 处理逻辑：直接生成结构体
                f.write(f"typedef struct {struct_name}_t\n")
                f.write("{\n")
                for field in structure["fields"]:
                    array = f"[{field['array_num']}]" if field["array_num"] > 1 else ""
                    type_map = {
                        1: "ZXIC_UINT8",
                        2: "ZXIC_UINT16",
                        4: "ZXIC_UINT32",
                        8: "ZXIC_UINT64"
                    }
                    field_type = type_map.get(field["element_size"], "ZXIC_UINT32")
                    f.write(f"    {field_type} {field['name']}{array};\n")
                f.write(f"}} ZXDH_{structure['name'].upper()}_T;\n\n")

                f.write(
                    f"ZXIC_UINT32 dpp_tbl_{structure['name']}_add(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index, ZXDH_{structure['name'].upper()}_T *p_Data);\n")
                f.write(
                    f"ZXIC_UINT32 dpp_tbl_{structure['name']}_del(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index);\n")
                f.write(
                    f"ZXIC_UINT32 dpp_tbl_{structure['name']}_get(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index, ZXDH_{structure['name'].upper()}_T *p_Data);\n\n")

            elif structure["type"] == "HASH":
                # HASH 处理逻辑：生成 key, entry 和整体结构体
                # Key 部分
                f.write(f"typedef struct {struct_name}_key\n")
                f.write("{\n")
                for field in structure["fields"]:
                    if field["attr"] == "key":
                        array = f"[{field['array_num']}]" if field["array_num"] > 1 else ""
                        type_map = {
                            1: "ZXIC_UINT8",
                            2: "ZXIC_UINT16",
                            4: "ZXIC_UINT32",
                            8: "ZXIC_UINT64"
                        }
                        field_type = type_map.get(field["element_size"], "ZXIC_UINT32")
                        f.write(f"    {field_type} {field['name']}{array};\n")
                f.write(f"}} ZXDH_{structure['name'].upper()}_KEY;\n\n")

                # Entry 部分
                f.write(f"typedef struct {struct_name}_entry\n")
                f.write("{\n")
                for field in structure["fields"]:
                    if field["attr"] == "rst":
                        array = f"[{field['array_num']}]" if field["array_num"] > 1 else ""
                        type_map = {
                            1: "ZXIC_UINT8",
                            2: "ZXIC_UINT16",
                            4: "ZXIC_UINT32",
                            8: "ZXIC_UINT64"
                        }
                        field_type = type_map.get(field["element_size"], "ZXIC_UINT32")
                        f.write(f"    {field_type} {field['name']}{array};\n")
                f.write(f"}} ZXDH_{structure['name'].upper()}_ENTRY;\n\n")

                # 整体结构体
                f.write(f"typedef struct {struct_name}_t\n")
                f.write("{\n")
                f.write(f"    ZXDH_{structure['name'].upper()}_KEY key;\n")
                f.write(f"    ZXDH_{structure['name'].upper()}_ENTRY entry;\n")
                f.write(f"}} ZXDH_{structure['name'].upper()}_T;\n\n")

                f.write(
                    f"ZXIC_UINT32 dpp_tbl_{structure['name']}_add(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_{structure['name'].upper()}_T *p_Data);\n")
                f.write(
                    f"ZXIC_UINT32 dpp_tbl_{structure['name']}_del(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_{structure['name'].upper()}_T *p_Data);\n")
                f.write(
                    f"ZXIC_UINT32 dpp_tbl_{structure['name']}_search(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_{structure['name'].upper()}_T *p_Data);\n\n")

            elif structure["type"] == "ACL":
                # ACL 处理逻辑：生成 key, mask, as_rlt 和整体结构体
                # Key 部分
                f.write(f"typedef struct {struct_name}_key\n")
                f.write("{\n")
                for field in structure["fields"]:
                    if field["attr"] == "key":
                        array = f"[{field['array_num']}]" if field["array_num"] > 1 else ""
                        type_map = {
                            1: "ZXIC_UINT8",
                            2: "ZXIC_UINT16",
                            4: "ZXIC_UINT32",
                            8: "ZXIC_UINT64"
                        }
                        field_type = type_map.get(field["element_size"], "ZXIC_UINT32")
                        f.write(f"    {field_type} {field['name']}{array};\n")
                f.write(f"}} ZXDH_{structure['name'].upper()}_KEY;\n\n")

                # Mask 部分
                f.write(f"typedef ZXDH_{structure['name'].upper()}_KEY ZXDH_{structure['name'].upper()}_MASK;\n\n")
                # As_rlt 部分
                f.write(f"typedef struct {struct_name}_as_rlt\n")
                f.write("{\n")
                for field in structure["fields"]:
                    if field["attr"] == "rst":
                        array = f"[{field['array_num']}]" if field["array_num"] > 1 else ""
                        type_map = {
                            1: "ZXIC_UINT8",
                            2: "ZXIC_UINT16",
                            4: "ZXIC_UINT32",
                            8: "ZXIC_UINT64"
                        }
                        field_type = type_map.get(field["element_size"], "ZXIC_UINT32")
                        f.write(f"    {field_type} {field['name']}{array};\n")
                f.write(f"}} ZXDH_{structure['name'].upper()}_AS_RLT;\n\n")

                # 整体结构体
                f.write(f"typedef struct {struct_name}_t\n")
                f.write("{\n")
                f.write(f"    ZXDH_{structure['name'].upper()}_KEY key;\n")
                f.write(f"    ZXDH_{structure['name'].upper()}_MASK mask;\n")
                f.write(f"    ZXDH_{structure['name'].upper()}_AS_RLT as_rlt;\n")
                f.write(f"}} ZXDH_{structure['name'].upper()}_T;\n\n")

                f.write(
                    f"ZXIC_UINT32 dpp_tbl_{structure['name']}_add(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle, ZXDH_{structure['name'].upper()}_T *p_{structure['name']});\n")
                f.write(
                    f"ZXIC_UINT32 dpp_tbl_{structure['name']}_del(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle);\n")
                f.write(
                    f"ZXIC_UINT32 dpp_tbl_{structure['name']}_get(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle, ZXDH_{structure['name'].upper()}_T *p_{structure['name']});\n")
                f.write(
                    f"ZXIC_UINT32 dpp_tbl_{structure['name']}_search(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle, ZXDH_{structure['name'].upper()}_T *p_{structure['name']});\n\n")

            # 写入尾部的头文件保护结束
            f.write("#endif\n")


def generate_acl_function(name, action):
    if action == "add":
        function_call = f"dpp_apt_dtb_acl_entry_insert_ex(&dev, queue, sdt_no, handle, p_{name})"
    elif action == "del":
        function_call = f"dpp_apt_dtb_acl_entry_del_ex(&dev, queue, sdt_no, handle)"
    elif action == "get":
        function_call = f"dpp_apt_dtb_acl_entry_get_ex(&dev, queue, sdt_no, handle, p_{name})"
    elif action == "search":
        function_call = f"dpp_apt_dtb_acl_entry_search_ex(&dev, queue, sdt_no, handle, p_{name})"
    else:
        raise ValueError("Invalid action for ACL")

    last_param = f", ZXDH_{name.upper()}_T *p_{name}" if action != "del" else ""
    check_point = f"ZXIC_COMM_CHECK_POINT(p_{name});" if action != "del" else ""

    template = f"""
ZXIC_UINT32 dpp_tbl_{name}_{action}(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle{last_param})
{{
    DPP_DEV_T dev = {{0}};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    {check_point}

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = {function_call};
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "{function_call.split('(')[0]}", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}}
EXPORT_SYMBOL(dpp_tbl_{name}_{action});
"""
    return template


def generate_eram_function(name, action):
    if action == "add":
        function_call = f"dpp_apt_dtb_eram_insert_ex(&dev, queue, sdt_no, index, pData)"
    elif action == "del":
        function_call = f"dpp_apt_dtb_eram_clear_ex(&dev, queue, sdt_no, index)"
    elif action == "get":
        function_call = f"dpp_apt_dtb_eram_get_ex(&dev, queue, sdt_no, index, pData)"
    else:
        raise ValueError("Invalid action for ERAM")

    template = f"""
ZXIC_UINT32 dpp_tbl_{name}_{action}(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index{", ZXDH_" + name.upper() + "_T *pData" if action != "del" else ""})
{{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 queue  = 0;
    DPP_DEV_T dev = {{0}};

    ZXIC_COMM_CHECK_POINT(pf_info);
    {"ZXIC_COMM_CHECK_POINT(pData);" if action != "del" else ""}

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = {function_call};
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "{function_call.split('(')[0]}", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}}
EXPORT_SYMBOL(dpp_tbl_{name}_{action});
"""
    return template


def generate_hash_function(name, action):
    if action == "add":
        function_call = f"dpp_apt_dtb_hash_insert_ex(&dev, queue, sdt_no, pData)"
    elif action == "del":
        function_call = f"dpp_apt_dtb_hash_delete_ex(&dev, queue, sdt_no, pData)"
    elif action == "search":
        function_call = f"dpp_apt_dtb_hash_search_ex(&dev, queue, sdt_no, pData)"
    else:
        raise ValueError("Invalid action for HASH")

    template = f"""
ZXIC_UINT32 dpp_tbl_{name}_{action}(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXDH_{name.upper()}_T* pData)
{{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 queue = 0;
    DPP_DEV_T dev = {{0}};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(pData);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = {function_call};
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "{function_call.split('(')[0]}", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}}
EXPORT_SYMBOL(dpp_tbl_{name}_{action});
"""
    return template


def generate_api_files(structures):
    # 用于存储已生成的结构体名称，避免重复生成
    generated_names = set()

    for struct in structures:
        type_ = struct['type']
        name = struct['name']

        # 检查是否已经生成该结构体
        if name in generated_names:
            continue
        generated_names.add(name)

        # 确定输出文件名
        output_file = f"{sys.argv[1]}/source/dpp_{name}_api.c".lower()

        with open(output_file, "w") as f:
            # 包含头文件
            f.write(f'#include "dpp_flow_comm.h"\n')
            f.write(f'#include "dpp_tbl_{name}.h"\n')
            f.write(f'#include "dpp_dev.h"\n\n')

            if type_ == "ACL":
                # 添加函数
                for action in ["add", "del", "get", "search"]:
                    f.write(generate_acl_function(name, action))
            elif type_ == "ERAM":
                # 添加函数
                for action in ["add", "del", "get"]:
                    f.write(generate_eram_function(name, action))
            elif type_ == "HASH":
                # 添加函数
                for action in ["add", "del", "search"]:
                    f.write(generate_hash_function(name, action))


def main():
    # 检查参数数量是否正确
    if len(sys.argv) != 2 or sys.argv[1] != "api":
        print("用法: python3 tool.py api")
        sys.exit(1)

    input_xml = f"zxdh_flow_attr_{sys.argv[1]}.xml"
    c_file = f"{sys.argv[1]}/source/dpp_flow_struct.c"

    structures = parse_flow_attr_xml(input_xml)

    generate_tbl_list_c_file(structures, c_file)
    generate_tbl_h_file(structures)
    generate_api_files(structures)


if __name__ == "__main__":
    main()
