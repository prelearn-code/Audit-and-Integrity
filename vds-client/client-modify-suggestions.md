# 函数
## 新增一个函数generateStateAssociatedToken()
计算公式：$\bar{T_i}=H_2(T_i||st_d)$
其中st_d是T_i对应关键词$w_i$的最新状态
函数输入：T_i st_d
输出：$\bar{T_i}$

## bool StorageClient::encryptFile(const std::string& file_path,
                               const std::vector<std::string>& keywords,
                               const std::string& output_prefix,
                               const std::string& insert_json_path) 

输入：加密文件 关键词集合
输出：output_prefix insert.json

其中insert.json中的T_i需要修改为$\bar{T_i}$
计算过程就是上述的generateStateAssociatedToken()的计算过程。
