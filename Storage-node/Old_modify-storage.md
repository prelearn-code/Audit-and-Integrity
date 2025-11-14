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
# 结构体
## 创建一个新的数据结构FileProof用在GetFileProof的中间变量
```C++
struct FileProof
{
    std::string psi;
    std::string phi;
}
```
# 修改函数
## SearchKeywordsAssociatedFilesProof
- 输入：不变
- 输出：JSON文件
修改为新的结构
```JSON
{
    "AS" : 
    [
        "2307241816711694767821996748735567293762182261177266528933182255418702765380"
    ],
    "PS" : 
    [
        {
            "ID_F" : "2307241816711694767821996748735567293762182261177266528933182255418702765380",
            "phi_alpha" : "5677f4e0ae41665db7d8d43e394b7b6f31ac4a67818c1c814e07aa7e9f2f56d8e065c66c6ebe5b8ab11294d1a81618c0c0556dfd610ba55af8886554e1a400f66f93615492acd8317381239f91b7fbd81c75fe4e7e4ae5c64e358a8e38dc443f27c19ccd9d147dd30bc8d1ff86b1e4e5b036881433f0156e7fdbf2075e6d6968",
            "psi_alpha" : "3c25e521c0d0f82b8684700d22423b28503c9a0451a8e81cca976058b269509a656f93d127fd4e460a1f98d16e9f9205a693a3fd4c0fa586cbe832cf52877ae3169c313663ddd439bfb3614170f6b4e910ae9dabf5314d7b3736a00944f281019a5a99d8dc250a631b5060feadf3b32bc4456700870d2974bbc530231a916538"
        }
    ],
    "T" : "591c66c69b47d7a3ad6fa5d8701eddf64a15ce753b15333409848821e979e32f",
    "std" : "ea4cedae3bbd8cba6ef4c82ee38d5ddf586320f37a5a8a244bdc10752a6b3002",
    "seed": "随机数的值",
    "phi":"phi的值"
}
```
- 计算过程：其他不变
    - 在整个操作开前，初始化一个phi变量为1。
    - 在执行操作1时，计算phi*= kt_wi,表示用索引搜索函数得到的结构的中kt_wi进行计算。
    - 在执行操作2时，修改psi_alpha初始化数值为0；
        - 在操作2执行中，修改计算函数psi_alpha*=prf_temp*C_(i,j)的更新方式为：psi_alpha+=prf_temp*C_(i,j)
    - 在结束时，把seed，phi也写入JSON文件。
# 函数实现
## 增加一个GetFileProof函数
- 输入：ID_F
- 输出：json文件，以ID_F命名。
- 文件：ID_F.json文件，在系统初始化时，创建"../data/FileProofs"文件夹
- 输出文件模板
    ```JSON
    {
        "ID_F":"文件ID",
        "FileProof":
        {
            "psi":"结构体FileProof的psi的数值",
            "phi":"结构体FileProof的phi的数值"
        },
        "seed":"随机数的数值"
    }
    ```
- 计算过程：
    - 加载索引数据库
    - 通过ID_F对于索引数据库进行索引，获取密文地址file_path,TS_F集合，其中TS_F中有n个元素。
    - 设置一个结构体变量FileProof fileproof;
    - 通过密文地址获取密文内容，通过区块大小BLOCK_SIZE与扇区大小SECTOR_SIZE对于密文文件进行拆分成为n个区块与s个扇区，其中n与TS_F里的元素个数相同。
    - 定义两个变量phi=1与psi=0
    - 对于密文的n个区块进行i从1-n的循环,同时循环TS_F
        - 设置变量seed,并计算seed=generate_random_seed(),得到一个随机数
        - 设置变量mpz_t result,并计算compute_prf(result,seed,ID_F,i)
        - 对于第i个密文块的s个扇区进行j从1-s循环
            - 计算psi+=result*c_(i,j) ，c_{i,j}为获取的密文的第i块第j个扇区的密文内容
        - 计算：phi *= (theta_i)^result,theta_i表示TS_F第i个元素值。
    - 最终将ID_F,proof，seed写入JSON文件
## 增加一个函数std::bool IsPairingEquation()
- 描述判断双线性映射的相等性，输入两个变量，输出1/0
- 输入：element_t input1,element_t input2;
- 输出：1/0
- 过程，判断两个输入的双线性映射结果是否相等。
## 增加一个VerifySearchProof函数
- 输入：JSON文件
- 标准文件
    ```JSON
    {
        "AS" : 
        [
            "2307241816711694767821996748735567293762182261177266528933182255418702765380"
        ],
        "PS" : 
        [
            {
             "ID_F" : "2307241816711694767821996748735567293762182261177266528933182255418702765380",
                "phi_alpha" : "5677f4e0ae41665db7d8d43e394b7b6f31ac4a67818c1c814e07aa7e9f2f56d8e065c66c6ebe5b8ab11294d1a81618c0c0556dfd610ba55af8886554e1a400f66f93615492acd8317381239f91b7fbd81c75fe4e7e4ae5c64e358a8e38dc443f27c19ccd9d147dd30bc8d1ff86b1e4e5b036881433f0156e7fdbf2075e6d6968",
                "psi_alpha" : "3c25e521c0d0f82b8684700d22423b28503c9a0451a8e81cca976058b269509a656f93d127fd4e460a1f98d16e9f9205a693a3fd4c0fa586cbe832cf52877ae3169c313663ddd439bfb3614170f6b4e910ae9dabf5314d7b3736a00944f281019a5a99d8dc250a631b5060feadf3b32bc4456700870d2974bbc530231a916538"
            }
        ],
        "T" : "591c66c69b47d7a3ad6fa5d8701eddf64a15ce753b15333409848821e979e32f",
        "std" : "ea4cedae3bbd8cba6ef4c82ee38d5ddf586320f37a5a8a244bdc10752a6b3002",
        "seed":"seed数值",
        "phi":"phi的值"
    }
    ```
- 输出：1/0 表示验证成功与失败
- 计算过程：
    - 加载输入文件的数据，AS,PS，T,std,seed,phi，其中AS中的元素数量与PS中的元素数量相等。
    - 首先加载索引数据库，获取ID_F对应的数据，获取密文的TS_F的元素个数n，获取公钥PK
    - 获取AS中的元素个数file_nums.
    - 设置变量：zeta_1,zeta_2都为1，zeta_3设置为phi的值,pho为0;
    - 对于PS进行循环，t从1-file_nums。
        - 定义mpz_t prf_temp;
        - 定义 element_t h2_temp_1,h2_temp_2;
        - 计算 computeHashH2(AS[t].ID_F,h2_temp_2),
        - 计算 zeta_2*=h2_temp_2；
        - 计算:zeta_3*=PS[t].phi_alpha；
        - 计算：pho+=PS[t].psi_alpha;
        - 对于n进行使用i从1-n进行遍历
            - 计算compute_prf(prf_temp,seed,PS[t].ID_F,i)；
            - 计算computeHashH2(AS[t].ID_F+i,h2_temp_1)；
            - 计算zeta_1*=h2_temp_1^prf_temp
    - 循环结束，得到zeta_1,zeta_2，zeta_3，pho的值。
    - 计算：left=(zeta_3,g) g是公共参数
    - 计算中间参数element_t Ti_bar_temp;
    - computeHashH2(std+T,Ti_bar_temp);
    - 计算：right = (zeta_1*zeta_2*Ti_bar_temp*(mu)^(pho),PK),其中mu是公共参数，pho为上面计算的值。
    - 验证过程：需要设计一个双线性匹配函数IsPairingEquation(left,right)
## 增加一个VerifyFileProof函数
- 函数描述：验证单个文件的证明的正确性
- 输入：JSON文件
    - 文件格式
    ```JSON
    {
        "ID_F":"文件ID",
        "FileProof":
        {
            "psi":"结构体FileProof的psi的数值",
            "phi":"结构体FileProof的phi的数值"
        },
        "seed":"随机数的数值"
    }
    ```
- 输出1/0，表示验证的成功与失败
- 函数计算过程：
    - 通过输入文件获取ID_F，psi,phi,seed
    - 加载索引数据库，索引ID_F数据，获取TS_F的元素个数n,公钥PK
    - 设置变量zeta=1
    - 循环i从1-n
        - 设置变量mpz_t ptr_temp;element_t h2_temp;
        - 计算compute_prf(ptr_temp,seed,ID_F,i)
        - 计算computeHashH2(ID_F+i,h2_temp)
        - 计算zeta*=h2_temp^{ptr_temp}
    - 循环结束得到zeta;
    - 设置element_t left,right;
    - 计算left = (phi,g) g为公共参数
    - 计算right=(zeta*mu^{psi},PK)
    - 通过设计的双线性映射函数IsPairingEquation(left,right)判断结果
    - 返回IsPairingEquation(left,right)的结果。