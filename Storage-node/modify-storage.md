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