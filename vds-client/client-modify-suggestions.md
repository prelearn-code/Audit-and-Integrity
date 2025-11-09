# 函数修改意见

## generateKeys()

### 输入

public_params.json公共参数

以公共参数作为输入，生成客户端本地自己的密钥相关参数。

### 输出

输出一个key文件作为私钥。

将公钥以及节点信息保存子public_key.json文件中。

## 加密文件函数

### 函数输入

文件地址

关键词集合（控制台逗号输入）

私钥（参数加载）

本地的关键词与对应状态的集合

### 函数输出

输出一个insert.json文件共storage node使用

输出个本地的加密文件同名的json，写入本次加密的相关参数信息。

下面是insert.json模板

```json
{
    "PK": "a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456",
    "ID_F": "file_001",
    "ptr": "encrypted_pointer_xyz123",
    "TS_F": "file_auth_tag_abc456",
    "state": "valid",
    "keywords": [
        {
            "T_i": "search_token_keyword1_hash",
            "kt_i": "keyword1"
        },
        {
            "T_i": "search_token_keyword2_hash",
            "kt_i": "keyword2"
        },
        {
            "T_i": "search_token_keyword3_hash",
            "kt_i": "keyword3"
        }
    ],
    "metadata": {
        "description": "示例文件",
        "tags": ["加密", "测试"],
        "original_filename": "example.txt"
    }
}
```

## 文件解密函数 （不用修改）

## 生成searchToken函数 与加密关键词函数重复了，只保留一个即可

### 输入

mk 主私钥

w 关键词

### 输出

T：searchtoken

### 本函数需要化简，只保留需要的计算部分，本函数的信息不用保存。

## generateAuthTags（TF_S生成）

加密逻辑有问题，对于认证标签的集合

应该是先把ID_F与块的索引i进行取或，然后去H_2的哈希值，即H_2(ID_F|i),再把区块分为s个扇区，每个扇区的密文块c_(i,j)，计算公共参数\mu的c_(i,j)次方，再将本区块的每个扇区的计算结果类成，再与H_2(ID_F|i)相乘，得到的结果再进行私钥sk签名，得到一个认证标签。

在对于每一块密文进行同样的处理，得到n个认证标签。

## genenrateKeywordTag

### 建议

修改名字为generate-state-associated-token

### 输入：

最新的状态：st_d

search token Ti

### 输出

generate-state-associated-token

## encryptPointer函数

### 输入：

当前最新状态

最新状态的哈希值

### 输出

ptr指针数据

## generateKeywordTag

### 输入

文件ID_F

当前关键词w_i

当前的关键词状态st_d

关键词的上一个状态st_(d-1)

私钥sk(通过系统加载)

### 输出

kt^(w_i)

### 计算过程

#### 情况1：存在上一个状态$st_d$

$$
kt^{w_i} = [H_2(ID_F)*H_2(st_d||T_i)/H_2(st_{d-1}||T_i)]^{sk}
$$


#### 情况2 不存在上一个状态$st_{d-1}$

关键词$w_i$的第一个文件。

$$
kt^{w_i} = [H_2(ID_F)*H_2(st_d||T_i]^{sk}
$$
