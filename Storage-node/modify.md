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

### save_index_database



