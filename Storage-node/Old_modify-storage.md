# 方案
## 修改结构体
- 修改前
    ```C++
        struct SearchResult {
        std::vector<std::string> ID_F;
        std::vector<std::string> keyword_proofs;
        std::string aggregated_proof;
    };
    ```
- 修改后,作为search中的中间搜索结构体
    ```C++
        struct SearchResult {
        std::string ID_F;
        std::string psi;
        std::string phi;
    }
    ```
## 修改函数
### 原函数
```C++
void StorageNode::compute_prf(mpz_t result, const std::string& seed, const std::string& input) {
    std::string combined = seed + input;
    std::string hash_hex = computeHashH3(combined);
    mpz_set_str(result, hash_hex.c_str(), 16);
    mpz_mod(result, result, N);
}
```
### 修改后
```C++
void StorageNode::compute_prf(mpz_t result, const std::string& seed, const std::string& ID_F,std::int index) {
    std::string combined = seed + input + std::to_string(index);
    scomputeHashH1(combined, result);
}
```

# 增加功能
## 增加变量（后面又其他用）
    inline static constexpr size_t BLOCK_SIZE = 4096;//把加密文件分块大小
    inline static constexpr size_t SECTOR_SIZE = 256;//一块的加密文件的细分为扇区的大小
    inline static constexpr size_t SECTORS_PER_BLOCK = BLOCK_SIZE / SECTOR_SIZE;//一个块的扇区个数
##  增加一个delete函数
输入：一个json文件地址,下面是JSON文件
```JSON
{
    "ID_F" : "2307241816711694767821996748735567293762182261177266528933182255418702765380",
    "PK" : "1bcbf90ebf29f101d3f17f3ee6880e3d291aadb4ba319b312b34d9cdeb937db406bb929605f16dfff11bb67551e06790151a0a1abe8091ad465c6406cdc29a7da50182f040537171b8364e0e83ca93a2742e72aa35b8c3d042f857a46efc7c6d5b2bec2a1c469013310e5cf37dfa28ea34bacf2621814634ad1e017364ac995e",
    "del" : "54581f93598f9950a61a1980e8b7ee0269890672f0c1441f795057fb759803731258ac4eaad8290573cebff7bbd52ba6e380918fab6c6df94a68956e3da25a01a1386647ccbc391dd6d7989212e845e2a8100502a8ee17c69c334bd16352dd9fe74eb0ada1daef9877b722705ecd01e4f1a22783c1e3bcf240513a55f30a08b3"
}
```
- 输出：删除成功/失败的提示即可
- 计算过程：
    - 首先加载索引数据库与搜索数据库
    - 根据ID_F在索引数据库找到对应的数据,若是PK与找到数据的PK相同，则进行下面操作
        - 循环本ID_F下面的keywords,找到所有的Ti_bar集合，用数组或者容器Ti_bars暂时存储，同时设置文件状态为invalid,并在循环keywords时，更新每一个kt_wi = kt_wi/del。
        -  根据上面找到的Ti_bar集合，Ti_bars，循环去根据Ti_bar去找到搜索数据库的每一个Ti_bar,更新每一个state为invalid,同时也更新kt_wi = kt_wi/del。
- 保存索引数据库与搜索数据库

## 增加SearchKeywordsAssociatedFilesProof函数
- 输入：JSON文件地址,下面是JSON文件格式
```JSON
{
    "PK" : "1bcbf90ebf29f101d3f17f3ee6880e3d291aadb4ba319b312b34d9cdeb937db406bb929605f16dfff11bb67551e06790151a0a1abe8091ad465c6406cdc29a7da50182f040537171b8364e0e83ca93a2742e72aa35b8c3d042f857a46efc7c6d5b2bec2a1c469013310e5cf37dfa28ea34bacf2621814634ad1e017364ac995e",
    "T" : "591c66c69b47d7a3ad6fa5d8701eddf64a15ce753b15333409848821e979e32f",
    "std" : "ea4cedae3bbd8cba6ef4c82ee38d5ddf586320f37a5a8a244bdc10752a6b3002"
}
```
- 输出：JSON文件
```JSON
{
    {
        "T":"searchToken",
        "std":"关键词对应的最新状态",
        "AS":["ID_F1","ID_F2"], //搜索涉及到的所有文件ID
        "PS":// 对应SearchResult结构体的集合
        [
            {
                "ID_F":"文件ID",
                "psi_alpha":"结构体对应的psi",
                "phi_alpha":"结构体对应的phi"
            }
            {
                "ID_F":"文件ID",
                "psi_alpha":"结构体对应的psi",
                "phi_alpha":"结构体对应的phi"
            }
        ]
    }
}
```
- 文件操作：
    - 系统启动时，创建./data/SearchProof文件夹
    - 每次的搜索结果用T.json命名，T是输入文件的T内容。
- 计算过程
    - 加载搜索数据库与索引数据库
    - 建立一个空的容器AS,用来存储涉及到的文件的ID_F
    - 读取数据JSON文件的T,std，PK数据
    - 设置一个PS容器，容器每个内容是三元组（ID_F,psi,phi）SearchResult结构体，这个容器包含多个这个三元组结构体
        - 可以设置数据结构的容器
            ```C++
            std::vector<SearchResult> Res,
            ```

    - 并设置一个当前状态st_alpha=std,与下一个文件下一个状态变量st_alpha_next
        - 操作1
            - 计算Ti_bar=computeHashH2(T+std)
            - 通过搜索数据库，索引Ti_bar的数据结构。
                - 得到：
                ```JSON
                {
                    "ID_F" : "2307241816711694767821996748735567293762182261177266528933182255418702765380",
                    "Ti_bar" : "2ed4b2e652a91043a03b82d9b9fb34ae60985b6e7d8b5c97962f70b018f3f6a83aee5543840c3e62536530a22ead391d6fb987adb1e57813ea9b57071a43f101777e5400da1b6ccddfa7c0f8c7f192e0c2a77632b146c57f1a6df8e8bb1592e50347a1ad5d8680de638587f5a52b91ad7ddc6d132938a598d4a484e00e97a0ca",
                    "kt_wi" : "54581f93598f9950a61a1980e8b7ee0269890672f0c1441f795057fb759803731258ac4eaad8290573cebff7bbd52ba6e380918fab6c6df94a68956e3da25a01a1386647ccbc391dd6d7989212e845e2a8100502a8ee17c69c334bd16352dd9fe74eb0ada1daef9877b722705ecd01e4f1a22783c1e3bcf240513a55f30a08b3",
                    "ptr_i" : "2ded234bd2d98435874afd6fed653390318f62732c16fe426c30e3d1f2306f321f86765ce889f71e2d342595a79aaa19b8ddde28cc0af25fb788f8542d15b793f4dc6ea357f3fbd5c668f823bde9f816",
                    "state" : "valid"
                }
                ```
            - 得到ID_F,通过索引数据库得到ID_F对应的文件地址与对应的公钥地址。
            - 公钥地址与输入的PK不相等，直接结这个函数的操作。
            - 计算std_alpha_next = decrypt_pointer(computeHashH3(st_alpha),st_alpha)
            - 把ID_F加入到AS容器
        - 操作2
            - 查看state状态，是valid，才进行下面操作，否则跳过本操作
            - 设置搜索结构体变量
                ```C++
                   temp_search_result;
                ```
            - 通过索引数据库获取到TS_F集合，假设有n个。
            - 索引得到文件的文件地址file_path
            - 变量赋值：ID_F添加到temp_search_result结构体中。
            - 生成一个随机数string类型的seed。
            - 计算psi_alpha，phi_alpha
                - 首先设置变量psi_alpha = 1，phi_alpha = 1
                - 通过第一步通过file_path加载密文C到内存中，
                - 其中按照变量设置的BLOCK_SIZE大小对于文件进行分块，分块数量与TS_F里面的变量数量相等，按照SECTOR_SIZE对于每一块分为s个小扇区。
                - 进行循环i从1-n
                    - 设置临时变量mpz_t::prf_temp;
                    -  调用函数compute_prf(prf_temp,seed,ID_F,i)
                    - 循环j从1-s
                        - psi_alpha*=prf_temp*C_(i,j) 把第i块的，第j个扇区的数据与prf_temp相乘（注意数据类型）
                    - 取TS_F的第i个数据sigma_i，计算sigam_i的prf_temp次方结果phi_temp;
                    - 计算phi_alpha*=phi_temp;
                - 循环结束
            - 得到psi_alpha，phi_alpha两个结果
            - 添加到temp_search_result
                - temp_search_result.psi = psi_alpha
                - temp_search_result.phi = phi_alpha
            把这个结构体temp_search_result加入到容器PS中
        
        - 操作3：
            - 判断st_aplha与std_alpha_next是否相等，来判断整个步操作1，2，3是否还要继续循环
            - 相等则结束
            - 不相等则赋值st_aplha=std_alpha_next，再执行1，2，3步骤
    - 上操作结束后，把T,std,AS,PS写入JSON文件中
