#ifndef DAMIAO_COMPILER_OPTIMIZATION_H
#define DAMIAO_COMPILER_OPTIMIZATION_H

/*
 * The firmware defaults to -Os.  Use the speed attribute only on measured
 * real-time hot paths whose generated code and observable behavior are covered
 * by the differential regressions.  Keeping the override on the function
 * avoids changing unrelated setup, diagnostics and protocol code in the same
 * translation unit.
 */
#if defined(__GNUC__) && !defined(__clang__)
#define DAMIAO_OPTIMIZE_SPEED __attribute__((optimize("O2")))
#else
#define DAMIAO_OPTIMIZE_SPEED
#endif

#endif
