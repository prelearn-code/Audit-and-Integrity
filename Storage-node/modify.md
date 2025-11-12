# 新的搜索使用的索引数据库

## 数据结构

### 新的结构体

```C++
struct IndexSearchEntry
{
  std::string Ti_bar:"插入文件的Ti_bar";
  std::string ID_F: "文件ID";//文件ID
  std::string ptr_i:"ptr_i";//关键词状态指针
  std::string state:"valid/invalid";//文件状态
  std::string kt_wi:"kt_wi";
}
```

其中，这些变量都跟insert.json的数据对应。

一个insert.json文件,对应多个IndexSearchEntry，其中的每个keywords对应的内容，都对应一个IndexSearchEntry结构体。

### 新的映射关系

设计新的映射关系

```C++
std::map<std::string,IndexSearchEntry>search_database;
```

其中，string对应的是Ti_bar, IndexSearchEntry对应结构体,以Ti_bar做为唯一标识


## 创建新的search_db.json

### 结构

```JSON
{
    {
        "Ti_bar": "Ti_bar的数值"，
        "ID_F":"文件ID的数值"，
        "ptr_i":"状态指针数值"，
        "state":"文件状态"，
        "kt_wi":"kt_wi"
     }
    {
        "Ti_bar": "Ti_bar的数值"，
        "ID_F":"文件ID的数值"，
        "ptr_i":"状态指针数值"，
        "state":"文件状态"，
        "kt_wi":"kt_wi"
     }
    {
        "Ti_bar": "Ti_bar的数值"，
        "ID_F":"文件ID的数值"，
        "ptr_i":"状态指针数值"，
        "state":"文件状态"，
        "kt_wi":"kt_wi"
     }
}
```

### 创建与更新这个文件

1. 创建：系统开始就创建，类似于index_db.json
2. 更新：首先插入文件操作时，需要把一个insert.json对应的多个IndexSearchEntry结构体插入到这个文件中，进行更新。
