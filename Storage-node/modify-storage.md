# 增加功能
## 增加变量（后面又其他用）
    inline static constexpr size_t BLOCK_SIZE = 4096;//把加密文件分块数量
    inline static constexpr size_t SECTOR_SIZE = 256;//一块的加密文件的细分为扇区
    inline static constexpr size_t SECTORS_PER_BLOCK = BLOCK_SIZE / SECTOR_SIZE;//一个扇区大小
##  增加一个delete函数
输入：

## 增加SearchKeywordsAssociatedFilesProof函数


## 增加一个GetFileProof函数

## 增加一个VerifySearchProof函数

## 增加一个VerifyFileProof函数