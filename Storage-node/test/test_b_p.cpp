#include <pbc/pbc.h>
#include <gmp.h>
#include <iostream>

int main() {
    // ====== 1. 初始化 pairing ======
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

    std::cout << "Pairing initialized.\n";

    // ====== 2. 在 G1 中测试 ======
    element_t g1, add_res, mul_res, id0, id1, acc_add, acc_mul;
    element_init_G1(g1, pairing);
    element_init_G1(add_res, pairing);
    element_init_G1(mul_res, pairing);
    element_init_G1(id0, pairing);
    element_init_G1(id1, pairing);
    element_init_G1(acc_add, pairing);
    element_init_G1(acc_mul, pairing);

    element_random(g1);  // 生成一个随机 G1 元素
    std::cout << "\n=== Test in G1 ===\n";
    std::cout << "Random g1 = ";
    element_printf("%B\n", g1);

    // 2.1 单位元：set0 与 set1 的一致性
    element_set0(id0);   // 对 G1 表示 identity
    element_set1(id1);   // 对 G1 也表示 identity（PBC 的统一约定）
    std::cout << "id0 = ";
    element_printf("%B\n", id0);
    std::cout << "id1 = ";
    element_printf("%B\n", id1);

    if (element_cmp(id0, id1) == 0) {
        std::cout << "[G1] identity: element_set0 == element_set1 ✅\n";
    } else {
        std::cout << "[G1] identity: element_set0 != element_set1 ❌\n";
    }

    // 2.2 一次运算：add 与 mul 是否一致？
    element_add(add_res, g1, g1);  // add_res = g1 + g1
    element_mul(mul_res, g1, g1);  // mul_res = g1 ⊕ g1（在 G1，这也是群运算）

    std::cout << "g1 + g1 = ";
    element_printf("%B\n", add_res);
    std::cout << "g1 * g1 (element_mul) = ";
    element_printf("%B\n", mul_res);

    if (element_cmp(add_res, mul_res) == 0) {
        std::cout << "[G1] add(g1,g1) == mul(g1,g1) ✅\n";
    } else {
        std::cout << "[G1] add(g1,g1) != mul(g1,g1) ❌\n";
    }

    // 2.3 多次累积：从单位元开始，add 累加 vs mul 累“乘”
    int k = 5;  // 测试 5 次
    element_set0(acc_add);  // identity
    element_set1(acc_mul);  // identity（在 G1 也是同一个 identity）

    for (int i = 0; i < k; ++i) {
        element_add(acc_add, acc_add, g1);  // acc_add = acc_add + g1
        element_mul(acc_mul, acc_mul, g1);  // acc_mul = acc_mul ⊕ g1（群运算）
    }

    std::cout << "k = " << k << "\n";
    std::cout << "acc_add (0 + k*g1) = ";
    element_printf("%B\n", acc_add);
    std::cout << "acc_mul (1 ⊕ g1 repeated k times) = ";
    element_printf("%B\n", acc_mul);

    if (element_cmp(acc_add, acc_mul) == 0) {
        std::cout << "[G1] k-times add == k-times mul from identity ✅\n";
    } else {
        std::cout << "[G1] k-times add != k-times mul ❌\n";
    }

    // ====== 3. 在 G2 中做同样测试（可选） ======
    element_t g2, add2, mul2, id0_2, id1_2;
    element_init_G2(g2, pairing);
    element_init_G2(add2, pairing);
    element_init_G2(mul2, pairing);
    element_init_G2(id0_2, pairing);
    element_init_G2(id1_2, pairing);

    element_random(g2);
    std::cout << "\n=== Test in G2 ===\n";
    std::cout << "Random g2 = ";
    element_printf("%B\n", g2);

    element_set0(id0_2);
    element_set1(id1_2);

    if (element_cmp(id0_2, id1_2) == 0) {
        std::cout << "[G2] identity: element_set0 == element_set1 ✅\n";
    } else {
        std::cout << "[G2] identity: element_set0 != element_set1 ❌\n";
    }

    element_add(add2, g2, g2);
    element_mul(mul2, g2, g2);

    if (element_cmp(add2, mul2) == 0) {
        std::cout << "[G2] add(g2,g2) == mul(g2,g2) ✅\n";
    } else {
        std::cout << "[G2] add(g2,g2) != mul(g2,g2) ❌\n";
    }

    // ====== 4. 清理 ======
    element_clear(g1);
    element_clear(add_res);
    element_clear(mul_res);
    element_clear(id0);
    element_clear(id1);
    element_clear(acc_add);
    element_clear(acc_mul);

    element_clear(g2);
    element_clear(add2);
    element_clear(mul2);
    element_clear(id0_2);
    element_clear(id1_2);

    pairing_clear(pairing);

    return 0;
}
