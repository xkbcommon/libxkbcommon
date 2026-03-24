configh_data.set10('HAVE_TOOLS', true)

tools_dep = declare_dependency(
    include_directories: include_directories(
        '..',  # required to #include "config.h" in subdirs
        '../include',
        '../src',
        '.',
    ),
    link_with: tools_shared_lib,
    dependencies: dep_libxkbcommon,
)

# Tool: export-keysyms
export_keysym_dep = [tools_dep]
if icu_dep.found()
    export_keysym_dep += [icu_dep]
endif
executable(
    'export-keysyms',
    'export-keysyms.c',
    link_with: libxkbcommon_test_internal,
    dependencies: export_keysym_dep,
    install: false,
)

# Tool: xkbcli
executable(
    'xkbcli',
    'xkbcli.c',
    dependencies: tools_dep,
    install: true,
)
install_man('xkbcli.1')

if get_option('enable-bash-completion')
    bash_completion_path = get_option('bash-completion-path')
    if bash_completion_path == ''
        bash_completion = dependency('bash-completion', required: false)
        if bash_completion.found()
            bash_completion_path = bash_completion.get_variable(
                pkgconfig: 'completionsdir',
            )
        else
            bash_completion_path = get_option('datadir') / 'bash-completion' / 'completions'
        endif
    endif
    install_data(
        'xkbcli-bash-completion.sh',
        rename: 'xkbcli',
        install_dir: bash_completion_path,
    )
endif

if get_option('enable-zsh-completion')
    zsh_completion_path = get_option('zsh-completion-path')
    if zsh_completion_path == ''
        zsh_completion_path = get_option('datadir') / 'zsh' / 'site-functions'
    endif
    install_data(
        'xkbcli-zsh-completion.zsh',
        rename: '_xkbcli',
        install_dir: zsh_completion_path,
    )
    install_data(
        'xkb-model-zsh-completion.zsh',
        rename: '_xkb_model',
        install_dir: zsh_completion_path,
    )
    install_data(
        'xkb-layout-zsh-completion.zsh',
        rename: '_xkb_layout',
        install_dir: zsh_completion_path,
    )
    install_data(
        'xkb-variant-zsh-completion.zsh',
        rename: '_xkb_variant',
        install_dir: zsh_completion_path,
    )
    install_data(
        'xkb-options-zsh-completion.zsh',
        rename: '_xkb_options',
        install_dir: zsh_completion_path,
    )
endif

# Tool: info
executable(
    'xkbcli-info',
    'info.c',
    dependencies: tools_dep,
    install: true,
    install_dir: dir_libexec,
)
install_man('xkbcli-info.1')
configh_data.set10('HAVE_XKBCLI_INFO', true)

# Tool: compile-keymap
xkbcli_compile_keymap = executable(
    'xkbcli-compile-keymap',
    'compile-keymap.c',
    dependencies: tools_dep,
    install: true,
    install_dir: dir_libexec,
)
install_man('xkbcli-compile-keymap.1')
configh_data.set10('HAVE_XKBCLI_COMPILE_KEYMAP', true)

# Tool: compose
executable(
    'xkbcli-compile-compose',
    'compile-compose.c',
    '../src/compose/dump.c',
    dependencies: tools_dep,
    install: true,
    install_dir: dir_libexec,
)
install_man('xkbcli-compile-compose.1')
configh_data.set10('HAVE_XKBCLI_COMPILE_COMPOSE', true)

# Tool: how-to-type
executable(
    'xkbcli-how-to-type',
    'how-to-type.c',
    dependencies: tools_dep,
    install: true,
    install_dir: dir_libexec,
)
install_man('xkbcli-how-to-type.1')
configh_data.set10('HAVE_XKBCLI_HOW_TO_TYPE', true)

# Tool: interactive-evdev
if cc.has_header('linux/input.h')
    executable(
        'xkbcli-interactive-evdev',
        'interactive-evdev.c',
        dependencies: tools_dep,
        install: true,
        install_dir: dir_libexec,
    )
    configh_data.set10('HAVE_XKBCLI_INTERACTIVE_EVDEV', true)
    install_man('xkbcli-interactive-evdev.1')
endif

# Tools: interactive-x11
if get_option('enable-x11')
    x11_tools_dep = declare_dependency(
        link_with: libxkbcommon_x11,
        dependencies: [
            tools_dep,
            xcb_dep,
            xcb_xkb_dep,
        ],
    )
    executable(
        'xkbcli-interactive-x11',
        'interactive-x11.c',
        dependencies: x11_tools_dep,
        install: true,
        install_dir: dir_libexec,
    )
    install_man('xkbcli-interactive-x11.1')
    configh_data.set10('HAVE_XKBCLI_INTERACTIVE_X11', true)

    executable(
        'xkbcli-dump-keymap-x11',
        'interactive-x11.c',
        dependencies: x11_tools_dep,
        c_args: ['-DKEYMAP_DUMP'],
        install: true,
        install_dir: dir_libexec,
    )
    install_man('xkbcli-dump-keymap-x11.1')
    configh_data.set10('HAVE_XKBCLI_DUMP_KEYMAP_X11', true)
endif

# Tools: interactive-wayland
if get_option('enable-wayland')
    wayland_client_dep = dependency(
        'wayland-client',
        version: '>=1.2.0',
        required: false,
    )
    wayland_protocols_dep = dependency(
        'wayland-protocols',
        version: '>=1.15',
        required: false,
    )
    wayland_scanner_dep = dependency(
        'wayland-scanner',
        required: false,
        native: true,
    )
    if not wayland_client_dep.found() or not wayland_protocols_dep.found() or not wayland_scanner_dep.found()
        error(
            '''The Wayland xkbcli programs require wayland-client and wayland-protocols which were not found.
You can disable the Wayland xkbcli programs with -Denable-wayland=false.''',
        )
    endif

    if cc.has_header_symbol('wayland/wayland-client-protocol.h', \
    'WL_KEYBOARD_KEY_STATE_REPEATED', \
    dependencies: wayland_client_dep)
        configh_data.set10('HAVE_WL_KEYBOARD_KEY_STATE_REPEATED', true)
    endif

    wayland_scanner = find_program(
        wayland_scanner_dep.get_variable(pkgconfig: 'wayland_scanner'),
    )
    wayland_scanner_code_gen = generator(
        wayland_scanner,
        output: '@BASENAME@-protocol.c',
        arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
    )
    wayland_scanner_client_header_gen = generator(
        wayland_scanner,
        output: '@BASENAME@-client-protocol.h',
        arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
    )
    wayland_protocols_datadir = wayland_protocols_dep.get_variable(
        pkgconfig: 'pkgdatadir',
    )
    xdg_shell_xml = wayland_protocols_datadir / 'stable' / 'xdg-shell' / 'xdg-shell.xml'
    xdg_shell_sources = [
        wayland_scanner_code_gen.process(xdg_shell_xml),
        wayland_scanner_client_header_gen.process(xdg_shell_xml),
    ]
    xdg_decoration_xml = wayland_protocols_datadir / 'unstable' / 'xdg-decoration' / 'xdg-decoration-unstable-v1.xml'
    xdg_decoration_sources = [
        wayland_scanner_code_gen.process(xdg_decoration_xml),
        wayland_scanner_client_header_gen.process(xdg_decoration_xml),
    ]
    executable(
        'xkbcli-interactive-wayland',
        'interactive-wayland.c',
        xdg_shell_sources,
        xdg_decoration_sources,
        dependencies: [tools_dep, wayland_client_dep],
        install: true,
        install_dir: dir_libexec,
    )
    install_man('xkbcli-interactive-wayland.1')
    configh_data.set10('HAVE_XKBCLI_INTERACTIVE_WAYLAND', true)

    executable(
        'xkbcli-dump-keymap-wayland',
        'interactive-wayland.c',
        xdg_shell_sources,
        xdg_decoration_sources,
        dependencies: [tools_dep, wayland_client_dep],
        c_args: ['-DKEYMAP_DUMP'],
        install: true,
        install_dir: dir_libexec,
    )
    install_man('xkbcli-dump-keymap-wayland.1')
    configh_data.set10('HAVE_XKBCLI_DUMP_KEYMAP_WAYLAND', true)
endif

# Tool: interactive
if (
    configh_data.get('HAVE_XKBCLI_INTERACTIVE_WAYLAND', 0) == 1
    or configh_data.get('HAVE_XKBCLI_INTERACTIVE_X11', 0) == 1
    or configh_data.get('HAVE_XKBCLI_INTERACTIVE_EVDEV', 0) == 1
)
    executable(
        'xkbcli-interactive',
        'interactive.c',
        dependencies: tools_dep,
        install: true,
        install_dir: dir_libexec,
    )
endif

# Tool: dump-keymap
if (
    configh_data.get('HAVE_XKBCLI_DUMP_KEYMAP_WAYLAND', 0) == 1
    or configh_data.get('HAVE_XKBCLI_DUMP_KEYMAP_X11', 0) == 1
)
    executable(
        'xkbcli-dump-keymap',
        'interactive.c',
        c_args: ['-DKEYMAP_DUMP'],
        dependencies: tools_dep,
        install: true,
        install_dir: dir_libexec,
    )
endif

# Tool: list
if get_option('enable-xkbregistry')
    configh_data.set10('HAVE_XKBCLI_LIST', true)
    executable(
        'xkbcli-list',
        'registry-list.c',
        dependencies: dep_libxkbregistry,
        include_directories: ['..', '.'],
        install: true,
        install_dir: dir_libexec,
    )
    install_man('xkbcli-list.1')
endif

# Tool: check-messages
executable(
    'xkb-check-messages',
    'check-messages.c',
    'messages.c',
    dependencies: [tools_dep],
    install: false,
)

# Tool: introspection
executable(
    'introspection',
    'introspection.c',
    '../src/utils.c',
    link_with: libxkbcommon_test_internal,
    dependencies: [tools_dep],
    install: false,
)

introspection_config = configuration_data()
introspection_config.set('MESON_BUILD_ROOT', meson.current_build_dir())
introspection_config.set('DEFAULT_XKB_RULES', get_option('default-rules'))
introspection_config.set('DEFAULT_XKB_MODEL', get_option('default-model'))
introspection_config.set('DEFAULT_XKB_LAYOUT', get_option('default-layout'))
introspection_config.set(
    'DEFAULT_XKB_VARIANT',
    get_option('default-variant'),
)
introspection_config.set(
    'DEFAULT_XKB_OPTIONS',
    get_option('default-options'),
)
configure_file(
    input: 'introspection-query.py.in',
    output: 'introspection-query',
    configuration: introspection_config,
)
