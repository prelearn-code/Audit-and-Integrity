import json
import os

def update_file_inplace(file_path, old_prefix, new_prefix):
    """
    直接读取并修改JSON文件，更新键中的路径前缀。
    """
    try:
        if not os.path.exists(file_path):
            print(f"错误: 找不到文件 '{file_path}'")
            return

        print(f"正在读取文件: {file_path} ...")
        with open(file_path, 'r', encoding='utf-8') as f:
            data = json.load(f)

        new_data = {}
        count = 0
        
        # 遍历并替换
        for key, value in data.items():
            if key.startswith(old_prefix):
                # 替换前缀
                new_key = key.replace(old_prefix, new_prefix, 1)
                new_data[new_key] = value
                count += 1
            else:
                new_data[key] = value

        print(f"处理完成，准备覆盖写入原文件。共更新 {count} 条路径。")

        # 覆盖写入原文件
        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(new_data, f, indent=2, ensure_ascii=False)
            
        print("修改成功！")

    except Exception as e:
        print(f"发生错误: {e}")

if __name__ == "__main__":
    # 目标文件
    filename = "/home/zsw/codes/Audit_TEST/claude_test/Audit-and-Integrity/make_data/database1_keywords.json"
    
    # 原有的前缀 (根据你上传的文件内容)
    old_path = "/home/zsw/codes/Audit-and-Integrity/make_data/database1/"
    
    # 新的前缀 (你刚刚指定的路径)
    # 注意：我在末尾添加了 '/' 以确保文件夹层级正确
    new_path = "/home/zsw/codes/Audit_TEST/claude_test/Audit-and-Integrity/make_data/database/1"

    # 执行
    update_file_inplace(filename, old_path, new_path)