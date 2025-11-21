#include<iostream>
#include <string>
#include <vector>
#include <pbc/pbc.h>
#include <gmp.h>
#include "storage_node.h"

void testParing(); 
void testVerify();
int main()
{
    //testParing();
    testVerify();
    return 0; 
}

void testVerify()
{
    std::cout << "Verify start" << std::endl;

    // 定义配对参数字符串
    const char* param_str = 
        "type a\n"
        "q 8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791\n"
        "h 12016012264891146079388821366740534204802954401251311822919615131047207289359704531102844802183906537786776\n"
        "r 730750818665451621361119245571504901405976559617\n"
        "exp2 159\n"
        "exp1 107\n"
        "sign1 1\n"
        "sign0 1\n";

    // 初始化配对参数
    pairing_t pairing;
    pairing_init_set_buf(pairing, param_str, strlen(param_str));



    // 需要的数据定义
    element_t g,mu,pk,phi,left,right;
    mpz_t psi,N;

    element_init_G1(g, pairing);
    element_init_G1(mu, pairing);
    element_init_G1(pk, pairing);
    element_init_G1(phi, pairing);
    element_init_GT(left, pairing);
    element_init_GT(right, pairing);

    mpz_init(psi);
    mpz_init(N);

    // 数据赋值

    std::string phi_hex = "5c237424aee8ee819943c80d2a3d0a1b04a7e1ffe240f953a936e71d2a4b101ddaa3a9469d8b860f8fa4b21c2386b0c97ecc68a8f20854213880965a3427f97b7a10e33f61a70c4282dd2154d3efcf7d537103251164ced6432c3555fed910cef68354b44f46d1f7dd1989018367de09185a6c9ca029c3eeb44d5f2a266d501b";
    std::string psi_hex = "2d3e3ca9921737fd32d4e34968d5c5238ed8c932fbc05de03223bc527b80735d9c956555fad0c28463cae87810eea5414f91f5109c3636799a9dbf5b51f38656fe163cd64e19aaba6e6728ddd94256982e9c01af8c83d0f84f5b45cf810fe15e1ee6f9f45f0bd126f5460e55079826876dee690cedba0202adfdde290b429b7e";
    std::string g_hex = "a0b41b546a2b80478d7f5e98f5ec150703a2fea61e69a5de9694b10bd8009a67461cafca84540ccf0e7d5170da267003308fd14de20cb5c6eaf913edbfe00697385d1115e98f5a0c91ac979dd153f6e52ea2271be39babab3cde10fc5613c09d3e442237a054e7458d98df69077e07bbf87f74322bacee29527c37dee33b3cf7";
    std::string mu_hex = "0e231739ec082c1972c9dcfc31351bcd2e8a44f5a94e370ec8eed3902402cef20ff24950713d29dd42c0549eb16c4706bedbebf519a73fe76e5231cf55ed400c9591efe922d84862ada73dab6d1ecf677e78483fe94dd54e1471aba4bfda571a59db52cc112348dfea6963d6105d290ceba7335aeaebeb7674f908eaefcf0bc9";
    std::string N_str = "77100882147323929259202707660697850182257322147504210450519405245425484999510534240288025446710114574437574014076458251591453146776013641689093802143952936303738599772833586128652280514157551668081419186182299772668216085721727589152216703947487484154379286657216379752967775433235508106221969798811366993681";
    std::string pk_hex = "a46a57789159d94f43a30967b25752f581f84cc3f0329da44085704954352874dc053d9a89f7da5a8eed9884bab98cd1e84c058f56fe64176e4cad20e2011e3100a9e37b565c6067ce0c1ec7d765796e727c02db758f577e937ed7b410bacb657fa6b875f1a83244527599e6a9c05a7d1134438232151f6ce90e6741cea2c78e";
    std::string ID_F = "15030926821912326204673978653733110398631717501691714889379196689817920475806";
    std::string seed = "5bfd4d11d36e8a34899879707a99cee72ad7a613f2ed462c339d39ee310e7d5a";

    StorageNode sn;
    // 反序列化
    sn.deserializeElement(g_hex,g);
    sn.deserializeElement(mu_hex,mu);
    sn.deserializeElement(phi_hex, phi);
    sn.deserializeElement(pk_hex, pk);
    mpz_set_str(psi, psi_hex.c_str(), 10);
    mpz_set_str(N, N_str.c_str(), 10);

    mpz_init(sn.N);
    mpz_set(sn.N, N);

    element_t zeta, h2_temp, temp_pow, mu_pow_psi;
    mpz_t prf_temp;

    int n = 5;
    for(int i = 0; i<n; i++)
    {
        sn.compute_prf(prf_temp, seed, ID_F, i);
        sn.computeHashH2(ID_F + std::to_string(i), h2_temp);
        element_init_G1(temp_pow, pairing);
        element_pow_mpz(temp_pow, h2_temp, prf_temp);
    }

    element_init_G1(mu_pow_psi, pairing);
    element_pow_mpz(mu_pow_psi, mu, psi);

    element_mul(right, zeta, mu_pow_psi);

    element_t right_pairing, left_pairing;
    element_init_GT(right_pairing, pairing);
    element_init_GT(left_pairing, pairing);
    pairing_apply(right_pairing, right, pk, pairing);
    pairing_apply(left_pairing, phi, g, pairing);
    int cmp = element_cmp(left, right_pairing);
    if(cmp == 0)
        std::cout << "Verify success" << std::endl;
    else
        std::cout << "Verify failed" << std::endl;      
}


void testParing()
{
    pbc_param_t param;
    pairing_t pairing;

    // 160-bit 群阶 r，512-bit 底域 q，大概是 80bit 安全级，实验足够用
    pbc_param_init_a_gen(param, 160, 512);

    pbc_param_out_str(stdout, param);



    pairing_init_pbc_param(pairing, param);
    
    std::cout << "✅ pairing 初始化成功" << std::endl;
    
    // G1 元素
    element_t g, gx, gy;
    element_init_G1(g,  pairing);
    element_init_G1(gx, pairing);
    element_init_G1(gy, pairing);
    
    // Zr 中的标量（指数）   
    element_t a, b, ab;
    element_init_Zr(a, pairing);
    element_init_Zr(b, pairing);
    element_init_Zr(ab, pairing);

    // GT 元素
    element_t e1, e2, e_gg;
    element_init_GT(e1,   pairing);
    element_init_GT(e2,   pairing);
    element_init_GT(e_gg, pairing);    

    // ========== 3. 随机取 g, a, b ==========
    element_random(g);   // G1 生成元（随机点即可）
    element_random(a);   // Zr 中随机
    element_random(b);   // Zr 中随机

    // 计算 ab = a * b （在 Zr 中）
    element_mul(ab, a, b);

    // ========== 4. 计算左边：e(g^a, g^b) ==========
    // gx = g^a, gy = g^b
    element_pow_zn(gx, g, a);  // g^a
    element_pow_zn(gy, g, b);  // g^b

    // e1 = e(g^a, g^b)
    pairing_apply(e1, gx, gy, pairing);

    // ========== 5. 计算右边：e(g,g)^(ab) ==========
    // e_gg = e(g,g)
    pairing_apply(e_gg, g, g, pairing);

    // e2 = e(g,g)^(ab)
    element_pow_zn(e2, e_gg, ab);
    int cmp = element_cmp(e1, e2);
    bool ok = (cmp == 0);
    if (ok) {
        std::cout << "✅ 双线性测试通过：e(g^a, g^b) = e(g,g)^(ab)" << std::endl;
    } else {
        std::cout << "❌ 双线性测试失败：e(g^a, g^b) ≠ e(g,g)^(ab)" << std::endl;
    }


    mpz_t phi, N, n1, n2;
    // 初始化一个大模数
    mpz_init_set_str(N, "77100882147323929259202707660697850182257322147504210450519405245425484999510534240288025446710114574437574014076458251591453146776013641689093802143952936303738599772833586128652280514157551668081419186182299772668216085721727589152216703947487484154379286657216379752967775433235508106221969798811366993681", 10);

    // 随机生成两个大整数 n1, n2 < N
    gmp_randstate_t state;
    gmp_randinit_default(state);
    gmp_randseed_ui(state, static_cast<unsigned long>(time(nullptr)));
    mpz_init(n1);
    mpz_init(n2);
    mpz_urandomm(n1, state, N);
    mpz_urandomm(n2, state, N);
    // ========== 6. 指数乘法测试：e(g^n1, g^n2) = e(g,g)^(n1*n2) ==========
    element_t e_n1, e_n2, e_n1n2, e_final1, e_final2;
    element_init_G1(e_n1, pairing);
    element_init_G1(e_n2, pairing);
    element_init_G1(e_n1n2, pairing);
    element_init_GT(e_final1, pairing);
    element_init_GT(e_final2, pairing);
    
    // 计算 g^n1 和 g^n2
    element_pow_mpz(e_n1, g, n1);
    element_pow_mpz(e_n2, g, n2);
    // 计算 g^(n1*n2)
    mpz_t n1n2;
    mpz_init(n1n2);
    mpz_mul(n1n2, n1, n2);
    element_pow_mpz(e_n1n2, g, n1n2);

    // 计算 e(g^n1, g^n2)
    pairing_apply(e_final1, e_n1, e_n2, pairing);
    // 计算 e(g,g)^(n1*n2)
    pairing_apply(e_gg, g, g, pairing);
    element_pow_mpz(e_final2, e_gg, n1n2);
    cmp = element_cmp(e_final1, e_final2);
    ok = (cmp == 0);
    if (ok) {   
        std::cout << "✅ 指数乘法测试通过：e(g^n1, g^n2) = e(g,g)^(n1*n2)" << std::endl;
    } else {
        std::cout << "❌ 指数乘法测试失败：e(g^n1, g^n2) ≠ e(g,g)^(n1*n2)" << std::endl;
    }   
    // ========== 7. 清理资源 ==========
    mpz_clear(n1);
    mpz_clear(n2);
    mpz_clear(n1n2);
    element_clear(e_n1);
    element_clear(e_n2); 
    element_clear(e_n1n2);
    element_clear(e_final1);
    element_clear(e_final2);
    element_clear(g);
    element_clear(gx);
    element_clear(gy);
    element_clear(a);
    element_clear(b);
    element_clear(ab);
    element_clear(e1);
    element_clear(e2);
    element_clear(e_gg);
    pairing_clear(pairing);
    pbc_param_clear(param);

}