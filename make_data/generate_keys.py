import requests
import json
import os
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, as_completed
from tqdm import tqdm
import threading

# ==================== 配置区 ====================
OLLAMA_URL = "http://localhost:11434/api/generate"
MODEL = "qwen2.5:7b"
DATABASE_DIR = "/home/zsw/codes/Audit-and-Integrity/make_data/database1"          # 数据库目录名
OUTPUT_FILE = "/home/zsw/codes/Audit-and-Integrity/make_data/database1_keywords.json"  # 输出JSON文件名
MAX_WORKERS = 5                     # 并发线程数（可调整：3-10）
MAX_CHARS = 3000                    # 每个文件读取的最大字符数
BATCH_SIZE = 300                    # 每处理多少个文件保存一次（降低内存占用）
# ===============================================

def ollama_generate(prompt, model=MODEL):
    """调用本地ollama生成关键词"""
    payload = {
        "model": model,
        "prompt": prompt,
        "stream": False
    }
    
    try:
        resp = requests.post(OLLAMA_URL, json=payload, timeout=120)
        resp.raise_for_status()
        return resp.json()["response"].strip()
    except Exception as e:
        return f"ERROR: {str(e)}"

def read_file(file_path, max_chars=MAX_CHARS):
    """读取文件内容（支持多种编码）"""
    for encoding in ['utf-8', 'gbk', 'gb2312', 'latin-1']:
        try:
            with open(file_path, 'r', encoding=encoding) as f:
                content = f.read(max_chars)
                return content
        except (UnicodeDecodeError, Exception):
            continue
    return None

def generate_keyword(file_path):
    """为单个文件生成关键词"""
    # 读取文件
    content = read_file(file_path)
    if not content:
        return None
    
    # 构建prompt
    prompt = f"""阅读后面的文本，然后提取出一个英文关键词，并只输出一个关键词，表示文章的主题。除了特定的人名与地名，都用小写表示。并且只输出为一个单一的英文单词，不能出现中文，或者其他多余的符号，只要一个单一的单词。
{content}"""
    
    # 调用ollama
    keyword = ollama_generate(prompt)
    return keyword if keyword else None

def get_file_iterator(database_dir):
    """生成器：逐个返回文件路径（不一次性加载所有路径）"""
    db_path = Path(database_dir)
    
    if not db_path.exists():
        raise FileNotFoundError(f"目录 {database_dir} 不存在！")
    
    # 使用生成器，不一次性加载所有路径到内存
    for root, dirs, filenames in os.walk(db_path):
        for filename in filenames:
            if not filename.startswith('.'):
                yield Path(root) / filename

def count_files(database_dir):
    """快速统计文件数量"""
    count = 0
    db_path = Path(database_dir)
    for root, dirs, filenames in os.walk(db_path):
        count += sum(1 for f in filenames if not f.startswith('.'))
    return count

def save_batch_to_json(batch_dict, output_file, is_first_batch=False):
    """增量写入JSON（追加模式）"""
    lock = threading.Lock()
    with lock:
        if is_first_batch:
            # 第一次写入：创建新文件
            with open(output_file, 'w', encoding='utf-8') as f:
                json.dump(batch_dict, f, ensure_ascii=False, indent=2)
        else:
            # 后续写入：读取现有内容，追加新内容
            try:
                with open(output_file, 'r', encoding='utf-8') as f:
                    existing_data = json.load(f)
            except (FileNotFoundError, json.JSONDecodeError):
                existing_data = {}
            
            existing_data.update(batch_dict)
            
            with open(output_file, 'w', encoding='utf-8') as f:
                json.dump(existing_data, f, ensure_ascii=False, indent=2)

def main():
    print("=" * 60)
    print("数据库关键词生成器（内存优化版）")
    print("=" * 60)
    
    # 1. 统计文件数量
    print(f"\n[1/3] 扫描目录: {DATABASE_DIR}")
    try:
        total_files = count_files(DATABASE_DIR)
        print(f"      找到 {total_files} 个文件")
    except FileNotFoundError as e:
        print(f"错误: {e}")
        return
    
    if total_files == 0:
        print("没有找到任何文件！")
        return
    
    # 2. 分批处理文件
    print(f"\n[2/3] 生成关键词 (并发数: {MAX_WORKERS}, 批次大小: {BATCH_SIZE})")
    
    file_iter = get_file_iterator(DATABASE_DIR)
    processed_count = 0
    success_count = 0
    error_count = 0
    batch_dict = {}
    is_first_batch = True
    
    with tqdm(total=total_files, desc="      处理进度", ncols=80) as pbar:
        with ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
            # 分批提交任务
            batch_files = []
            for file_path in file_iter:
                batch_files.append(file_path.absolute())
                
                # 达到批次大小，开始处理
                if len(batch_files) >= BATCH_SIZE:
                    # 处理当前批次
                    future_to_file = {
                        executor.submit(generate_keyword, fp): fp 
                        for fp in batch_files
                    }
                    
                    for future in as_completed(future_to_file):
                        file_path = future_to_file[future]
                        try:
                            keyword = future.result()
                            if keyword:
                                if keyword.startswith("ERROR"):
                                    error_count += 1
                                else:
                                    success_count += 1
                                batch_dict[str(file_path)] = keyword
                        except Exception as e:
                            batch_dict[str(file_path)] = f"ERROR: {str(e)}"
                            error_count += 1
                        pbar.update(1)
                    
                    # 保存当前批次到JSON
                    save_batch_to_json(batch_dict, OUTPUT_FILE, is_first_batch)
                    is_first_batch = False
                    batch_dict = {}
                    batch_files = []
            
            # 处理最后一个不完整的批次
            if batch_files:
                future_to_file = {
                    executor.submit(generate_keyword, fp): fp 
                    for fp in batch_files
                }
                
                for future in as_completed(future_to_file):
                    file_path = future_to_file[future]
                    try:
                        keyword = future.result()
                        if keyword:
                            if keyword.startswith("ERROR"):
                                error_count += 1
                            else:
                                success_count += 1
                            batch_dict[str(file_path)] = keyword
                    except Exception as e:
                        batch_dict[str(file_path)] = f"ERROR: {str(e)}"
                        error_count += 1
                    pbar.update(1)
                
                if batch_dict:
                    save_batch_to_json(batch_dict, OUTPUT_FILE, is_first_batch)
    
    # 3. 完成统计
    print(f"\n[3/3] 保存完成: {OUTPUT_FILE}")
    print("\n" + "=" * 60)
    print(f"完成！")
    print(f"  总文件数: {total_files}")
    print(f"  成功生成: {success_count}")
    print(f"  失败/错误: {error_count}")
    print(f"  输出文件: {Path(OUTPUT_FILE).absolute()}")
    print("=" * 60)

if __name__ == "__main__":
    main()