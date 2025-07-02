#include "regs/cheshire.h"
#include "dif/clint.h"
#include "dif/uart.h"
#include "params.h"
#include "util.h"
#include "workload.h"

uint64_t sN = 402;
volatile __attribute__((aligned(8))) double c[NN];

volatile int __attribute__((noinline)) _payload(double *a, double *b, double *c, uint64_t N, uint64_t M, uint64_t K,
                                                uint64_t a_rowstride, uint64_t b_colstride, uint64_t c_rowstride)
{
    for (int m = 0; m < M; m += 4)
        for (int n = 0; n < N; n += 4)
            for (int k = 0; k < K; k += 4) {
                uint64_t a_row     = m;
                uint64_t b_col     = n;
                uint64_t c_row     = m;
                uint64_t a_coloffs = k;
                uint64_t b_rowoffs = k;
                uint64_t c_coloffs = n;
                double cb[16]      = {0};
                for (int row = 0; row < 4; ++row) {
                    double *cx  = &cb[row * 4];
                    double *ax  = &a[(a_row + row) * a_rowstride + a_coloffs];
                    double *bb0 = &b[(b_col + 0) * b_colstride + b_rowoffs];
                    double *bb1 = &b[(b_col + 1) * b_colstride + b_rowoffs];
                    double *bb2 = &b[(b_col + 2) * b_colstride + b_rowoffs];
                    double *bb3 = &b[(b_col + 3) * b_colstride + b_rowoffs];
                    asm volatile(
                        // stream b cols and compute
                        "fld  f0,  0(%[bb0]) \n"
                        "fld  f1,  8(%[bb0]) \n"
                        "fld  f2, 16(%[bb0]) \n"
                        "fld  f3, 24(%[bb0]) \n"
                        "fld  f4,  0(%[bb1]) \n"
                        "fld  f5,  8(%[bb1]) \n"
                        "fld  f6, 16(%[bb1]) \n"
                        "fld  f7, 24(%[bb1]) \n"
                        "fmadd.d %[cx0], %[ax0], f0, %[cx0] \n"
                        "fmadd.d %[cx1], %[ax1], f1, %[cx1] \n"
                        "fmadd.d %[cx2], %[ax2], f2, %[cx2] \n"
                        "fmadd.d %[cx3], %[ax3], f3, %[cx3] \n"
                        "fmadd.d %[cx0], %[ax0], f4, %[cx0] \n"
                        "fmadd.d %[cx1], %[ax1], f5, %[cx1] \n"
                        "fmadd.d %[cx2], %[ax2], f6, %[cx2] \n"
                        "fmadd.d %[cx3], %[ax3], f7, %[cx3] \n"
                        "fld  f0,  0(%[bb2]) \n"
                        "fld  f1,  8(%[bb2]) \n"
                        "fld  f2, 16(%[bb2]) \n"
                        "fld  f3, 24(%[bb2]) \n"
                        "fld  f4,  0(%[bb3]) \n"
                        "fld  f5,  8(%[bb3]) \n"
                        "fld  f6, 16(%[bb3]) \n"
                        "fld  f7, 24(%[bb3]) \n"
                        "fmadd.d %[cx0], %[ax0], f0, %[cx0] \n"
                        "fmadd.d %[cx1], %[ax1], f1, %[cx1] \n"
                        "fmadd.d %[cx2], %[ax2], f2, %[cx2] \n"
                        "fmadd.d %[cx3], %[ax3], f3, %[cx3] \n"
                        "fmadd.d %[cx0], %[ax0], f4, %[cx0] \n"
                        "fmadd.d %[cx1], %[ax1], f5, %[cx1] \n"
                        "fmadd.d %[cx2], %[ax2], f6, %[cx2] \n"
                        "fmadd.d %[cx3], %[ax3], f7, %[cx3] \n"
                        : [bb0] "+&r"(bb0), [bb1] "+&r"(bb1), [bb2] "+&r"(bb2), [bb3] "+&r"(bb3), [cx0] "+&f"(cx[0]),
                          [cx1] "+&f"(cx[1]), [cx2] "+&f"(cx[2]), [cx3] "+&f"(cx[3]), [ax0] "+&f"(ax[0]),
                          [ax1] "+&f"(ax[1]), [ax2] "+&f"(ax[2]), [ax3] "+&f"(ax[3])::"f0", "f1", "f2", "f3", "f4",
                          "f5", "f6", "f7");
                }
                c[(c_row + 0) * c_rowstride + (c_coloffs + 0)] = cb[0];
                c[(c_row + 0) * c_rowstride + (c_coloffs + 1)] = cb[1];
                c[(c_row + 0) * c_rowstride + (c_coloffs + 2)] = cb[2];
                c[(c_row + 0) * c_rowstride + (c_coloffs + 3)] = cb[3];
                c[(c_row + 1) * c_rowstride + (c_coloffs + 0)] = cb[4];
                c[(c_row + 1) * c_rowstride + (c_coloffs + 1)] = cb[5];
                c[(c_row + 1) * c_rowstride + (c_coloffs + 2)] = cb[6];
                c[(c_row + 1) * c_rowstride + (c_coloffs + 3)] = cb[7];
                c[(c_row + 2) * c_rowstride + (c_coloffs + 0)] = cb[8];
                c[(c_row + 2) * c_rowstride + (c_coloffs + 1)] = cb[9];
                c[(c_row + 2) * c_rowstride + (c_coloffs + 2)] = cb[10];
                c[(c_row + 2) * c_rowstride + (c_coloffs + 3)] = cb[11];
                c[(c_row + 3) * c_rowstride + (c_coloffs + 0)] = cb[12];
                c[(c_row + 3) * c_rowstride + (c_coloffs + 1)] = cb[13];
                c[(c_row + 3) * c_rowstride + (c_coloffs + 2)] = cb[14];
                c[(c_row + 3) * c_rowstride + (c_coloffs + 3)] = cb[15];
            }
    return 0;
}

int main(void)
{
    const uint64_t num_steps = 7;

    for (uint64_t step = 4; step < num_steps; step = step + 1) {
        asm volatile("fence" ::: "memory");

        // payload bay -> for a nop 25 seconds
        volatile int dump;
        for (int r = 0; r < 1 * (1 + step); ++r) {
            dump = _payload(float_data_a, float_data_b, c, sN, sN, sN, sN, sN, sN);
        }
    }
    return 0;
}
