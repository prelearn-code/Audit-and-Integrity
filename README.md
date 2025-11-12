# 合约地址与客户端对应

## 合约地址

VDSCore: 0x1fAD0219bF042F81CBD65b4F8dbc859F4EdAe4dc
VDSSearch: 0x45a8d7bfCB9a4D8EDd9615dBC9329ea806657D7E
VDSVerification: 0xB11585D4B7F77b4652a4C2fD8cD0DB331c4Ef092

## 客户端地址

## 服务端地址

### storage node 1

private key: f7112b77d591a809c61e36db086ee252d7fa280e286e0150a60d9e25d016ece3
public key: 0x2e02dc838d8a2b1b3cf6597f4ac2a50659647f8b
passwd : 000000


# 修改方案
# 函数实现问题

## 数据库相关的
### 数据库设计
```JSON
{
  "last_update":"times",//最新更新时间，精确到秒
  "file_count": 3,                          // 新增：文件总数
  "ID_Fs": ["ID_F1", "ID_F2", "ID_F3"],   // 新增：文件ID索引列表
  "database": [                            // 修改：原来的索引数据
    {
      "ID_F": "58596621420790973770...",   // 文件唯一标识
      "file_path": "enc.enc",              // 加密文件存储路径
      "PK": "932fec9942585339b445...",     // 客户端公钥
      "TS_F": ["988ffd60...", "7068fe..."], // 文件认证标签集合
      "keywords": [                        // 关键词索引
        {
          "Ti_bar": "a4d04362...",         // 状态令牌
          "kt_wi": "33faaf63...",          // 关键词标签
          "ptr_i": "addceb6b..."           // 状态指针
        }
      ],
      "state": "valid"                     // 文件状态
    }
  ]
}
```

### load_index_database
根据新设计的数据库，进行读取文件到内存。
项目启动时加载。

### save_index_database
重新设计函数
输入 文件 ID_F，结构体 IndexEntry(插入操作使用)，string del（删除操作使用），操作类型 insert/delete
首先要需要判断数据库是否加载到内存
函数内容分为两类
总体：last_update：更新为最新的时间
  insert操作
    file_count:加1
    ID_Fs：增加新文件的ID_F
    database:添加新文件的相关信息，就是IndexEntry的数据
  delete:
    file_count:加-1
    ID_Fs：删除文件的ID_F
    database中的操作
      设置state状态为 invalid
      本文件的所有kt_wi = kt_wi/del
然后保存到存储到数据库文件。
但是还是保留内存的数据库更新后的信息
返回插入/删除的结果是否完成0/1