#include <pbc/pbc.h>
#include <gmp.h>
#include <iostream>

int main() {
    // ====== 1. 初始化 pairing：使用你的 Type A 参数 ======
    const char* param_str =
        "type a\n"
        "q 8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791\n"
        "h 12016012264891146079388821366740534204802954401251311822919615131047207289359704531102844802183906537786776\n"
        "r 730750818665451621361119245571504901405976559617\n"
        "exp2 159\n"
        "exp1 107\n"
        "sign1 1\n"
        "sign0 1\n";

    pairing_t pairing;
    pairing_init_set_str(pairing, param_str);

    // ====== 2. 提取群阶 r ======
    mpz_t r;
    mpz_init_set_str(r,
        "730750818665451621361119245571504901405976559617", 10);

    // ====== 3. 初始化 G1 元素 ======
    element_t g, v1, g_sk, v1_sk;
    element_init_G1(g, pairing);
    element_init_G1(v1, pairing);
    element_init_G1(g_sk, pairing);
    element_init_G1(v1_sk, pairing);

    // ====== 4. 初始化 GT 元素 ======
    element_t left, right;
    element_init_GT(left, pairing);
    element_init_GT(right, pairing);

    // ====== 5. 随机生成 g, v1 ======
    element_random(g);
    element_random(v1);

    std::cout << "g   = "; element_printf("%B\n", g);
    std::cout << "v1  = "; element_printf("%B\n", v1);

    // ====== 6. 生成随机 sk in Z_r ======
    gmp_randstate_t state;
    gmp_randinit_default(state);

    mpz_t seed;
    mpz_init_set_ui(seed, 12345);
    gmp_randseed(state, seed);

    mpz_t sk;
    mpz_init(sk);
    mpz_urandomm(sk, state, r);

    std::cout << "sk = ";
    mpz_out_str(stdout, 10, sk);
    std::cout << std::endl;

    // ====== 7. 计算 g^sk 和 v1^sk（标量乘法）=====
    element_pow_mpz(g_sk, g, sk);
    element_pow_mpz(v1_sk, v1, sk);

    std::cout << "g^sk  = "; element_printf("%B\n", g_sk);
    std::cout << "v1^sk = "; element_printf("%B\n", v1_sk);

    // ====== 8. left = e(v1, g^sk) ======
    pairing_apply(left, v1, g_sk, pairing);
    std::cout << "left  = e(v1, g^sk) = ";
    element_printf("%B\n", left);

    // ====== 9. right = e(v1^sk, g) ======
    pairing_apply(right, v1_sk, g, pairing);
    std::cout << "right = e(v1^sk, g) = ";
    element_printf("%B\n", right);

    // ====== 10. 比较 left 和 right ======
    if (element_cmp(left, right) == 0) {
        std::cout << "\n✅ Test passed: e(v1, g^sk) == e(v1^sk, g)\n";
    } else {
        std::cout << "\n❌ Test FAILED: values do not match.\n";
    }

    // 清理资源
    element_clear(g);
    element_clear(v1);
    element_clear(g_sk);
    element_clear(v1_sk);

    element_clear(left);
    element_clear(right);

    mpz_clear(r);
    mpz_clear(sk);
    mpz_clear(seed);
    gmp_randclear(state);

    pairing_clear(pairing);

    return 0;
}
