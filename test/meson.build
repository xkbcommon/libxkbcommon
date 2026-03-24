test_env = environment()
test_env.set('XKB_LOG_LEVEL', 'debug')
test_env.set('XKB_LOG_VERBOSITY', '10')
test_env.set('top_srcdir', meson.project_source_root())
test_env.set('top_builddir', meson.project_build_root())
test_env.set(
    'HAVE_XKBCLI_INTERACTIVE_EVDEV',
    configh_data.get('HAVE_XKBCLI_INTERACTIVE_EVDEV', 0).to_string(),
)
test_env.set(
    'HAVE_XKBCLI_INTERACTIVE_WAYLAND',
    configh_data.get('HAVE_XKBCLI_INTERACTIVE_WAYLAND', 0).to_string(),
)
test_env.set(
    'HAVE_XKBCLI_INTERACTIVE_X11',
    configh_data.get('HAVE_XKBCLI_INTERACTIVE_X11', 0).to_string(),
)
test_env.set(
    'HAVE_XKBCLI_LIST',
    configh_data.get('HAVE_XKBCLI_LIST', 0).to_string(),
)

test_configh_data = configuration_data()
test_configh_data.set_quoted(
    'TEST_XKB_CONFIG_ROOT',
    meson.current_source_dir() / 'data',
)
test_configh_data.set('_FORTIFY_SOURCE', '3')
configure_file(output: 'test-config.h', configuration: test_configh_data)

# Headers C++ compatibility test (optional)
# Exclude MSVC because it does not handle mixing C and C++ well
if build_machine.system() != 'windows' and add_languages(
    'cpp',
    required: false,
    native: false,
)
    cpp_compat_test_deps = []
    cpp_compat_test_cpp_args = []
    if get_option('enable-x11')
        cpp_compat_test_cpp_args += '-DHAS_XKB_X11'
        cpp_compat_test_deps += xcb_dep
    endif
    if get_option('enable-xkbregistry')
        cpp_compat_test_cpp_args += '-DHAS_XKB_REGISTRY'
    endif
    test(
        'headers C++ compatibility',
        executable(
            'cpp-headers',
            'headers-compatibility.cpp',
            include_directories: include_directories(
                '..',  # required to #include "config.h" in subdirs
                '../include',
            ),
            dependencies: cpp_compat_test_deps,
            cpp_args: cpp_compat_test_cpp_args,
            override_options: ['cpp_std=c++11'],
        ),
    )
endif

m_dep = cc.find_library('m', required: false)
test_shared_lib = static_library(
    'test-shared',
    sources: [
        '../bench/bench.c',
        '../src/utils.c',
        'common.c',
        'compose-iter.c',
        'utils-text.c',
    ],
    include_directories: include_directories(
        '..',  # required to #include "config.h" in subdirs
        '../src',
        '../include',
        '.',
    ),
    link_with: [libxkbcommon_test_internal, tools_shared_lib],
)

test_dep = declare_dependency(
    include_directories: include_directories(
        '..',  # required to #include "config.h" in subdirs
        '../src',
        '../include',
        '.',
    ),
    link_with: [libxkbcommon_test_internal, test_shared_lib],
    dependencies: [m_dep],
)

test(
    'features',
    executable(
        'features',
        'features.c',
        dependencies: test_dep,
        c_args: ['-DENABLE_PRIVATE_APIS'],
    ),
    env: test_env,
)
# TODO: version range?
keysyms_test_dep = [test_dep]
keysyms_test_c_args = ['-DENABLE_PRIVATE_APIS']
if icu_dep.found()
    keysyms_test_dep += [icu_dep]
    configh_data.set10('HAVE_ICU', true)
endif
test(
    'keysym',
    executable(
        'keysym',
        'keysym.c',
        dependencies: keysyms_test_dep,
        c_args: keysyms_test_c_args,
    ),
    env: test_env,
)
test(
    'keysym-unicode',
    executable(
        'keysym-unicode',
        'keysym-unicode.c',
        dependencies: keysyms_test_dep,
        c_args: keysyms_test_c_args,
    ),
    env: test_env,
)
test(
    'keymap',
    executable('keymap', 'keymap.c', dependencies: test_dep),
    env: test_env,
)
test(
    'filecomp',
    executable('filecomp', 'filecomp.c', dependencies: test_dep),
    env: test_env,
)
test(
    'context',
    executable('context', 'context.c', dependencies: test_dep),
    env: test_env,
)
test(
    'rules-file',
    executable('rules-file', 'rules-file.c', dependencies: test_dep),
    env: test_env,
)
test(
    'rules-file-includes',
    executable(
        'rules-file-includes',
        'rules-file-includes.c',
        dependencies: test_dep,
    ),
    env: test_env,
)
test(
    'stringcomp',
    executable('stringcomp', 'stringcomp.c', dependencies: test_dep),
    env: test_env,
)
test(
    'buffercomp',
    executable('buffercomp', 'buffercomp.c', dependencies: test_dep),
    env: test_env,
)
test(
    'lenient-mode',
    executable(
        'lenient-mode',
        'lenient-mode.c',
        dependencies: test_dep,
    ),
    env: test_env,
)
if has_extensions_directories
    extensions_test_deps = [test_dep]
    extensions_test_c_args = []
    if get_option('enable-xkbregistry')
        extensions_test_deps += [dep_libxkbregistry, dep_libxml]
        extensions_test_c_args += ['-DHAS_XKBREGISTRY']
    endif
    test(
        'extensions-directories',
        executable(
            'extensions-directories',
            'extensions-directories.c',
            dependencies: extensions_test_deps,
            c_args: extensions_test_c_args,
        ),
        env: test_env,
    )
endif
test(
    'introspection',
    executable(
        'introspection',
        'introspection.c',
        dependencies: test_dep,
    ),
    env: test_env,
)

# The merge modes tests are optional and generated locally on-the-fly
pymod = import('python')
python = pymod.find_installation('python3', modules: ['jinja2'], required: false)
has_merge_modes_tests = python.found() and python.language_version().version_compare(
    '>=3.11',
)
if has_merge_modes_tests
    script = find_program('../scripts/update-merge-modes-tests.py')
    merge_modes_tests = [meson.project_name()]
    # NOTE: The following tests deal with third-party XKB compilers and are
    #       reserved for debugging purposes. Uncomment to enable them.
    # merge_modes_tests += 'xkbcomp' # Xorg XKB compiler
    # merge_modes_tests += 'kbvm'    # Jay compositor XKB compiler
    foreach xkb_compiler : merge_modes_tests
        if xkb_compiler == meson.project_name()
            suffix = ''
            extra_args = []
        else
            suffix = '-' + xkb_compiler
            extra_args = [xkb_compiler]
        endif
        filename = f'merge-modes@suffix@.c'
        merge_modes_c = custom_target(
            filename,
            build_by_default: true,
            command: [
                script,
                '--root',
                meson.project_source_root(),
                '--c-out',
                '@OUTPUT@',
                '--no-xkb',
            ] + extra_args,
            depend_files: ['merge_modes.c.jinja'],
            output: filename,
            capture: false,
            install: false,
        )
        test(
            f'merge-modes@suffix@',
            executable(
                f'merge-modes@suffix@',
                merge_modes_c,
                dependencies: test_dep,
            ),
            env: test_env,
        )
    endforeach
endif

test(
    'log',
    executable('log', 'log.c', dependencies: test_dep),
    env: test_env,
)
test(
    'atom',
    executable('atom', 'atom.c', dependencies: test_dep),
    env: test_env,
)
test(
    'utf8',
    executable('utf8', 'utf8.c', dependencies: test_dep),
    env: test_env,
)
test(
    'state',
    executable('state', 'state.c', dependencies: test_dep),
    env: test_env,
)
test(
    'server-state',
    executable(
        'server-state',
        'server-state.c',
        dependencies: test_dep,
    ),
    env: test_env,
)
test(
    'keyseq',
    executable('keyseq', 'keyseq.c', dependencies: test_dep),
    env: test_env,
)
test(
    'rulescomp',
    executable('rulescomp', 'rulescomp.c', dependencies: test_dep),
    env: test_env,
)
test(
    'compose',
    executable(
        'compose',
        'compose.c',
        '../src/compose/dump.c',
        dependencies: test_dep,
    ),
    env: test_env,
)
test(
    'utils',
    executable('utils', 'utils.c', dependencies: test_dep),
    env: test_env,
)
test(
    'symbols-leak-test',
    find_program('symbols-leak-test.py'),
    env: test_env,
    suite: ['python-tests'],
)
test(
    'modifiers',
    executable('modifiers', 'modifiers.c', dependencies: test_dep),
    env: test_env,
)
test(
    'messages',
    executable(
        'messages',
        'messages.c',
        '../tools/messages.c',
        include_directories: include_directories('../tools'),
        dependencies: test_dep,
    ),
    env: test_env,
)
if get_option('enable-x11')
    has_xvfb = find_program('Xvfb', required: false)
    has_xkbcomp = find_program('xkbcomp', required: false)
    # We only warn because the build machine may not be the same
    # as the host/test machine.
    if not has_xvfb.found()
        warning('Xvfb program not found, but is required to run X11 tests.')
    endif
    if not has_xkbcomp.found()
        warning('xkbcomp program not found, but is required to run X11 tests.')
    endif

    x11_test_dep = declare_dependency(
        link_with: libxkbcommon_x11_test_internal,
        dependencies: [
            test_dep,
            xcb_dep,
        ],
    )
    x11_xvfb_test_shared_lib = static_library(
        'x11-xvfb-test-shared',
        sources: [
            'xvfb-wrapper.c',
        ],
        dependencies: x11_test_dep,
    )
    x11_xvfb_test_dep = declare_dependency(
        link_with: x11_xvfb_test_shared_lib,
        dependencies: [x11_test_dep],
    )
    test(
        'x11',
        executable('x11', 'x11.c', dependencies: x11_xvfb_test_dep),
        suite: ['x11'],
        env: test_env,
        is_parallel: false,
    )
    test(
        'x11comp',
        executable(
            'x11comp',
            'x11comp.c',
            dependencies: x11_xvfb_test_dep,
        ),
        suite: ['x11', 'flaky'],
        env: test_env,
        is_parallel: false,
    )
endif
if get_option('enable-xkbregistry')
    test(
        'registry',
        executable(
            'registry',
            'registry.c',
            dependencies: [dep_libxkbregistry, dep_libxml, test_dep],
        ),
        env: test_env,
    )
endif
if build_tools
    test(
        'tool-option-parsing',
        find_program('tool-option-parsing.py'),
        env: test_env,
        suite: ['python-tests'],
    )

    # A set of keysyms to test for. Add one or two symbols to this array
    # whenever the xorgproto gets updated to make sure we resolve them.
    keysyms_to_test = [
        'XF86Macro23',
    ]

    env = environment()
    env.set('XKB_CONFIG_ROOT', meson.current_source_dir() / 'data')
    foreach keysym : keysyms_to_test
        test(
            'keysym-@0@'.format(keysym),
            find_program('test-keysym.py'),
            env: env,
            args: [keysym, '--tool', xkbcli_compile_keymap],
            suite: ['python-tests'],
        )
    endforeach
endif

valgrind = find_program('valgrind', required: false)
if valgrind.found()
    add_test_setup(
        'valgrind',
        exe_wrapper: [
            valgrind,
            '--leak-check=full',
            '--track-origins=yes',
            '--gen-suppressions=all',
            '--error-exitcode=99',
        ],
        # This is used in some tests, to avoid excessive run time.
        env: {'RUNNING_VALGRIND': '1'},
        timeout_multiplier: 10,
    )
else
    message('valgrind not found, disabling valgrind test setup')
endif


# xkeyboard-config "verifier"
xkct_config = configuration_data()
xkct_config.set('MESON_BUILD_ROOT', meson.current_build_dir())
xkct_config.set('XKB_CONFIG_ROOT', XKB_CONFIG_ROOT)
configure_file(
    input: 'xkeyboard-config-test.py.in',
    output: 'xkeyboard-config-test',
    configuration: xkct_config,
)
