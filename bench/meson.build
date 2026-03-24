bench_env = environment()
bench_env.set('top_srcdir', meson.project_source_root())
benchmark(
    'key-proc',
    executable('key-proc', 'key-proc.c', dependencies: test_dep),
    env: bench_env,
)
benchmark(
    'keysym-case-mappings',
    executable(
        'keysym-case-mappings',
        'keysym-case-mappings.c',
        dependencies: test_dep,
        c_args: ['-DENABLE_PRIVATE_APIS'],
    ),
)
benchmark(
    'rulescomp',
    executable('rulescomp', 'rulescomp.c', dependencies: test_dep),
    env: bench_env,
)
if cc.has_header_symbol('getopt.h', 'getopt_long', prefix: '#define _GNU_SOURCE')
    benchmark(
        'rules',
        executable('rules', 'rules.c', dependencies: test_dep),
        env: bench_env,
    )
    benchmark(
        'compile-keymap',
        executable(
            'compile-keymap',
            'compile-keymap.c',
            dependencies: test_dep,
        ),
        env: bench_env,
    )
    benchmark(
        'dump-keymap',
        executable(
            'dump-keymap',
            'compile-keymap.c',
            dependencies: test_dep,
            c_args: ['-DKEYMAP_DUMP'],
        ),
        env: bench_env,
    )
    benchmark(
        'custom-parsers',
        executable(
            'custom-parsers',
            'custom-parsers.c',
            dependencies: test_dep,
        ),
        env: bench_env,
    )
endif
benchmark(
    'compose',
    executable('compose', 'compose.c', dependencies: test_dep),
    env: bench_env,
)
benchmark(
    'compose-traversal',
    executable(
        'compose-traversal',
        'compose-traversal.c',
        dependencies: test_dep,
    ),
    env: bench_env,
)
benchmark(
    'atom',
    executable('atom', 'atom.c', dependencies: test_dep),
    env: bench_env,
)
if get_option('enable-x11')
    benchmark(
        'x11',
        executable('x11', 'x11.c', dependencies: x11_test_dep),
        env: bench_env,
    )
endif
