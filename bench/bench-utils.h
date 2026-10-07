/*
 * SPDX-License-Identifier: MIT
 *
 * ---------------------------------------------------------------------------
 * API
 * ---------------------------------------------------------------------------
 * bench_do_not_optimize(value)
 *     Requires an *lvalue*. Evaluate `value` exactly once and force the result
 *     to be materialized: the compiler must assume the (empty) asm reads it, so
 *     the computation cannot be dead-coded.
 *
 * bench_opaque_input(value)
 *     Requires a *modifiable lvalue*. The compiler must assume the asm
 *     re-wrote it, so its value becomes unknown: defeats constant folding
 *     through `value`. Apply to *inputs* before the timed region (or inside
 *     the loop to defeat hoisting).
 *
 * bench_clobber_memory()
 *     Compiler-level memory barrier: memory ops cannot move across it.
 *     Compiler-only: emits no instructions, is not a CPU fence and provides
 *     no thread synchronization.
 *
 * ---------------------------------------------------------------------------
 * EXAMPLE
 * ---------------------------------------------------------------------------
 *   static uint32_t BENCH_NOINLINE my_hash(uint32_t x);
 *
 *   void bench_my_hash(void) {
 *       uint32_t x = 0x12345678, acc = 0;
 *       bench_opaque_input(x);                 // input: not a constant
 *       for (int i = 0; i < N; i++) {
 *           acc += my_hash(x ^ (uint32_t)i);   // vary inputs per iteration
 *       }
 *       bench_do_not_optimize(acc);            // output is observable
 *   }
 *
 * ---------------------------------------------------------------------------
 * SUPPORT MATRIX (arch first, compilers second)
 * ---------------------------------------------------------------------------
 * The asm backend uses only generic "r"/"m" operand constraints with an empty
 * template — no arch-specific assembly anywhere, so it works on every native
 * GCC/Clang target: x86, x86-64, ARM32, AArch64, RISC-V, MIPS, PowerPC,
 * s390x, LoongArch, m68k, AVR, MSP430, ... (GCC >= 5 or any modern Clang).
 *
 * Targets where register/memory asm operands do not exist are routed to the
 * portable fallback even under GCC/Clang: WebAssembly (Emscripten, wasi-sdk),
 * PNaCl. This mirrors google/benchmark, which does the same for
 * BENCHMARK_HAS_NO_INLINE_ASSEMBLY.
 *
 * All other compilers (MSVC, ...) get the fallback: an observable volatile
 * store of the object's address into a sink variable (C11 5.1.2.3: volatile
 * accesses are observable side effects, so nothing can delete them).
 *
 * ---------------------------------------------------------------------------
 * IMPLEMENTATION FOOTGUNS (all are intentional, none are silent)
 * ---------------------------------------------------------------------------
 * F1. BENCH_KEEP does NOT prevent constant folding. If the compiler can fold
 *     the whole expression to a compile-time constant, it will (and then just
 *     feeds the constant to the asm). Make inputs opaque first with
 *     BENCH_OPAQUE, or declare inputs volatile. This is why google/benchmark
 *     deprecated DoNotOptimize(T const&) with the message "can permit
 *     undesired compiler optimizations; store it in a local variable first".
 *
 * F2. Loop-invariant hoisting (LICM) still applies: the asm pins *where the
 *     value is consumed*, not where it is computed. A pure computation on
 *     loop-invariant inputs can be hoisted above your loop and computed once.
 *     Fix: vary the inputs per iteration (see `x ^ (uint32_t)i` above), or
 *     re-apply BENCH_OPAQUE to them inside the loop.
 *
 * F3. BENCH_BARRIER()/the "memory" clobber reorder only what the *compiler*
 *     does. There is no CPU fence, and no synchronization between threads.
 *
 * F4. GCC/Clang path uses GNU extension __asm__ but -pedantic
 *     stays quiet. On the fallback path the code is pure ISO C11.
 *
 * F5. BENCH_OPAQUE requires a modifiable lvalue. Passing a const-qualified
 *     variable or a bit-field fails to compile — loudly, on purpose (a const
 *     value cannot be "made unknown", a bit-field cannot be an asm operand).
 *     For const values, copy to a local first, or use BENCH_KEEP on it.
 *
 * F6. Fallback compilers (MSVC, wasm, ...): the semantics are weaker:
 *     dead-code elimination is prevented (address of the object escapes
 *     through an observable volatile store), but constant folding is NOT
 *     defeated. If you need folding protection there, declare the input itself
 *     volatile.
 *
 * F7. GPU/offload code: this header is host-only. nvcc defines __GNUC__, but
 *     device code has no GNU asm — you will get a loud compile error if you
 *     try, which is the intended behavior. Keep benchmarks on the host.
 *
 * F8. Large structs are fine ("m" alternative keeps them in memory; nothing
 *     must fit in a register). Very old compilers are untested: if some
 *     pre-2015 GCC rejects a large operand, define BENCH_FORCE_FALLBACK.
 *
 * F9. The barrier itself is free for the asm path: the asm template is empty,
 *     so zero instructions are emitted. The only cost is the materialization
 *     you explicitly asked for.
 *
 * F10. The "memory" clobber only affects memory the compiler considers
 *     observable (globals, escaped pointers). Non-escaped locals can still
 *     be moved or kept in registers.
 *
 * Optional: -DBENCH_FORCE_FALLBACK forces the portable backend.
 */

#pragma once

#include "config.h"

#if (defined(__GNUC__) || defined(__clang__)) \
    && !defined(BENCH_FORCE_FALLBACK)         \
    && !defined(__EMSCRIPTEN__)               \
    && !defined(__wasm__)                     \
    && !defined(__pnacl__)

/*******************************************************************************
 * Primary backend: empty GCC-style asm (GCC, Clang, ICX, ...)                 *
 *******************************************************************************/

#define BENCH_NOINLINE __attribute__((noinline))

#define BENCH_KEEP(value) __asm__ volatile ("" : : "r,m"(value) : "memory")

/**
 * Make an lvalue unknown to the compiler: the read-write constraint makes the
 * compiler forget its value (defeats constant folding) and must have computed
 * it. Zero cost, stays in register for pointers/ints.
 */
#if defined(__clang__)
#define BENCH_OPAQUE(v) __asm__ volatile ("" : "+r,m"(v) : : "memory")
#else
#define BENCH_OPAQUE(v) __asm__ volatile ("" : "+m,r"(v) : : "memory")
#endif

/** The asm operands already require a modifiable lvalue for BENCH_OPAQUE. */
#define BENCH_CHECK_MODIFIABLE(v) ((void) 0)

/**
 * Compiler-level memory barrier: compiler assumes any memory may have
 * been read/written. Emits no code.
 */
#define BENCH_BARRIER() __asm__ volatile ("" : : : "memory")

#else

/*******************************************************************************
 * Fallback backend: observable volatile store; standard C only, best-effort.  *
 ******************************************************************************/

#if defined(_MSC_VER)
#define BENCH_NOINLINE __declspec(noinline)
#else
#define BENCH_NOINLINE
#endif

static inline void
bench_noop_(void)
{
}

/** Weak barrier: a volatile read+write the compiler cannot remove. */
static inline void
bench_clobber_memory_impl_(void)
{
    /* The compiler must assume an unknown function may touch escaped memory */
    static void (*volatile fn)(void) = bench_noop_;
    fn();
}

/*
 * Fallback semantics (see F6): taking the address of `var` and storing it
 * into a volatile sink is an observable side effect, so `var` must exist in
 * memory and whatever computed its value must be kept. Weaker than the asm
 * backend: no constant-folding protection, lvalue arguments only.
 */
static inline void
bench_escape_impl_(const void *p)
{
    static const void *volatile sink;
    /* volatile store: observable side effect */
    sink = p;
    bench_clobber_memory_impl_();
    (void)sink;
}

#define BENCH_KEEP(v) bench_escape_impl_(&(v))

#define BENCH_OPAQUE(v) BENCH_KEEP(v)

#define BENCH_BARRIER() bench_clobber_memory_impl_()

/* Unevaluated: rejects const lvalues, like the gcc "+r,m" operand does. */
#define BENCH_CHECK_MODIFIABLE(v) ((void) sizeof((v) = (v)))

#endif

/*******************************************************************************
 * Public API                                                                  *
 ******************************************************************************/

#define bench_do_not_optimize(value) do {\
    /* must be an lvalue */              \
    (void) sizeof(&(value));             \
    BENCH_KEEP(value);                    \
} while (0)

#define bench_opaque_input(value) do {\
    /* must be an lvalue */           \
    (void) sizeof(&(value));          \
    BENCH_CHECK_MODIFIABLE(value);    \
    BENCH_OPAQUE(value);              \
} while (0)

#define bench_clobber_memory() do { \
    BENCH_BARRIER();                \
} while (0)
