#include<iostream>
#include <string>
#include <vector>
#include <pbc/pbc.h>
#include <gmp.h>

void testParing(); 

int main()
{
    testParing();
    return 0; 
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


    mpz_t n1, n2;
    mpz_init_set_d(n1, 123456789);
    mpz_init_set_d(n2, 987654321);

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