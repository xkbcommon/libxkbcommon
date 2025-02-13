if cc.get_id() == 'clang'

targets = [
    'keymap'
]

configure_file(output: 'config.h', configuration: configh_data)

foreach target : targets

    exe = executable(
        f'fuzz-@target@-parser',
        f'@target@/parser.c',
        include_directories: include_directories('../src', '../include'),
        dependencies: [
            test_dep,
        ],
        install: false,
        c_args : ['-g', '-fsanitize=fuzzer,address,undefined'],
        link_args : ['-g', '-fsanitize=fuzzer,address,undefined']
    )
    # TODO: Drop?
    exe = executable(f'fuzz-@target@', f'@target@/compile.c',
        dependencies: [
            dep_libxkbcommon,
        ],
        install: false,
        c_args : '-fsanitize=fuzzer,address,undefined',
        link_args : '-fsanitize=fuzzer,address,undefined'
    )
    exe = executable(f'fuzz-@target@-main', f'@target@/target.c',
        dependencies: [
            dep_libxkbcommon,
        ],
        install: false,
        # implicit_include_directories: false,
        c_args : '-fsanitize=fuzzer-no-link,address',
        link_args : '-fsanitize=fuzzer-no-link,address'
    )

endforeach

endif
