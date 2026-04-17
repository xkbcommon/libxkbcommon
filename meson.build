project(
    'libxkbcommon',
    'c',
    version: '1.13.0',
    default_options: [
        'c_std=c11',
        'build.c_std=c11',
        'warning_level=3',
    ],
    meson_version: '>= 1.4.0',  # Released on March 2024
)

xkbcommon_project_version = meson.project_version().split('.')
if xkbcommon_project_version[0] != '1'
    # The versioning used for the shared libraries assumes that the major
    # version of xkbcommon as a whole will increase to 2 if and only if there
    # is an ABI break, at which point we should probably bump the SONAME of
    # all libraries to .so.2.
    error('We probably need to bump the SONAME of libxkbcommon')
else
    # To avoid an unnecessary SONAME bump, xkbcommon 1.x.y produces
    # libxkbcommon.so.0.x.y, libxkbcommon-x11.so.0.x.y, libxkbregistry.so.0.x.y.
    soname_version = '.'.join(
        ['0', xkbcommon_project_version[1], xkbcommon_project_version[2]],
    )
endif

pkgconfig = import('pkgconfig')
cc = meson.get_compiler('c')

dir_libexec = get_option('prefix') / get_option('libexecdir') / 'xkbcommon'

# Compiler flags.
cflags = [
    '-fno-strict-aliasing',
    '-Wno-unused-parameter',
    '-Wno-missing-field-initializers',
    '-Wpointer-arith',
    '-Wmissing-declarations',
    '-Wformat=2',
    '-Wstrict-prototypes',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wbad-function-cast',
    '-Wshadow',
    '-Wlogical-op',
    '-Wdate-time',
    '-Wwrite-strings',
    '-Wno-documentation-deprecated-sync',
    '-Wno-pedantic',
    '/utf-8',
    '/wd4100',  # Disable MSVC C4100: unreferenced formal parameter
]
add_project_arguments(cc.get_supported_arguments(cflags), language: 'c')

# The legacy X11 config root. Only xkeyboard-config 2.* provides it.
# This is intended when using old xkeyboard-config and as a last resort for
# misconfigured setups.
# Try both un- and versioned pkg-config (xkeyboard-config < 2.45).
XKB_LEGACY_ROOT = ''
foreach v : ['-2', '']
    xkeyboard_config_dep = dependency(f'xkeyboard-config@v@', required: false)
    if xkeyboard_config_dep.found()
        XKB_LEGACY_ROOT = xkeyboard_config_dep.get_variable(
            pkgconfig: 'xkb_base',
            default_value: '',
        )
        break
    endif
endforeach
if XKB_LEGACY_ROOT == ''
    # Fallback to the legacy X11 directory
    XKB_LEGACY_ROOT = get_option('prefix') / get_option('datadir') / 'X11' / 'xkb'
endif

# The xkeyboard-config root
# Try to set it from the xkeyboard-config package, from the most recent to the
# oldest major version. The hard-coded major versions list should cover a few
# decades, given the expected pace of xkeyboard-config format changes.
XKEYBOARD_CONFIG_ROOT = ''
XKEYBOARD_CONFIG_VERSION_MAJOR = ''
XKEYBOARD_CONFIG_VERSIONED_EXTENSIONS_PATH = ''
XKEYBOARD_CONFIG_UNVERSIONED_EXTENSIONS_PATH = ''
foreach v : [12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2]
    xkeyboard_config_dep = dependency(f'xkeyboard-config-@v@', required: false)
    if xkeyboard_config_dep.found()
        XKEYBOARD_CONFIG_ROOT = xkeyboard_config_dep.get_variable(
            pkgconfig: 'xkb_root',
        )
        XKEYBOARD_CONFIG_VERSION_MAJOR = xkeyboard_config_dep.version().split(
            '.',
        )[0]
        # The following variables may not be defined
        XKEYBOARD_CONFIG_VERSIONED_EXTENSIONS_PATH = xkeyboard_config_dep.get_variable(
            pkgconfig: 'xkb_versioned_extensions_path',
            default_value: '',
        )
        XKEYBOARD_CONFIG_UNVERSIONED_EXTENSIONS_PATH = xkeyboard_config_dep.get_variable(
            pkgconfig: 'xkb_unversioned_extensions_path',
            default_value: '',
        )
        break
    endif
endforeach
if XKEYBOARD_CONFIG_ROOT == ''
    # Try old unversioned pkg-config (xkeyboard-config < 2.45)
    xkeyboard_config_dep = dependency('xkeyboard-config', required: false)
    if xkeyboard_config_dep.found()
        XKEYBOARD_CONFIG_ROOT = xkeyboard_config_dep.get_variable(
            pkgconfig: 'xkb_base',
        )
        XKEYBOARD_CONFIG_VERSION_MAJOR = xkeyboard_config_dep.version().split(
            '.',
        )[0]
        # No extension path defined
    endif
endif

# The XKB config root for libxkbcommon
XKB_CONFIG_ROOT = get_option('xkb-config-root')
if XKB_CONFIG_ROOT == ''
    if XKEYBOARD_CONFIG_ROOT != ''
        XKB_CONFIG_ROOT = XKEYBOARD_CONFIG_ROOT
    else
        # No xkeyboard-config or too old: use legacy X11 path fallback
        XKB_CONFIG_ROOT = XKB_LEGACY_ROOT
    endif
endif

# The XKB extensions directories
XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH = get_option(
    'xkb-config-unversioned-extensions-path',
)
if XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH == '' and XKEYBOARD_CONFIG_ROOT != ''
    if XKEYBOARD_CONFIG_UNVERSIONED_EXTENSIONS_PATH != ''
        XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH = XKEYBOARD_CONFIG_UNVERSIONED_EXTENSIONS_PATH
    elif XKEYBOARD_CONFIG_ROOT.endswith(f'-@XKEYBOARD_CONFIG_VERSION_MAJOR@')
        # The xkeyboard-config root is versioned: remove the version suffix
        # Meson does not have a length() function… Compute the length manually
        s = XKEYBOARD_CONFIG_VERSION_MAJOR
        foreach len : range(3)
            if s == ''
                break
            endif
            s = s.substring(1)
        endforeach
        XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH = XKEYBOARD_CONFIG_ROOT.substring(
            0,
            -len - 1,
        ) + '.d'
    else
        XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH = get_option('prefix') / get_option(
            'datadir',
        ) / 'xkeyboard-config.d'
    endif
endif
XKB_CONFIG_VERSIONED_EXTENSIONS_PATH = get_option(
    'xkb-config-versioned-extensions-path',
)
if XKB_CONFIG_VERSIONED_EXTENSIONS_PATH == '' and XKEYBOARD_CONFIG_ROOT != ''
    if XKEYBOARD_CONFIG_VERSIONED_EXTENSIONS_PATH != ''
        XKB_CONFIG_VERSIONED_EXTENSIONS_PATH = XKEYBOARD_CONFIG_VERSIONED_EXTENSIONS_PATH
    else
        XKB_CONFIG_VERSIONED_EXTENSIONS_PATH = f'@XKEYBOARD_CONFIG_ROOT@.d'
    endif
endif

XKB_CONFIG_EXTRA_PATH = get_option('xkb-config-extra-path')
if XKB_CONFIG_EXTRA_PATH == ''
    XKB_CONFIG_EXTRA_PATH = get_option('prefix') / get_option('sysconfdir') / 'xkb'
endif

# The X locale directory for compose.
XLOCALEDIR = get_option('x-locale-root')
if XLOCALEDIR == ''
    XLOCALEDIR = get_option('prefix') / get_option('datadir') / 'X11' / 'locale'
endif


# config.h.
configh_data = configuration_data()
configh_data.set('EXIT_INVALID_USAGE', '2')
configh_data.set_quoted('LIBXKBCOMMON_VERSION', meson.project_version())
configh_data.set_quoted('LIBXKBCOMMON_TOOL_PATH', dir_libexec)
# Like AC_USE_SYSTEM_EXTENSIONS, what #define to use to get extensions
# beyond the base POSIX function set.
if host_machine.system() == 'sunos'
    system_extensions = '__EXTENSIONS__'
else
    system_extensions = '_GNU_SOURCE'
endif
configh_data.set(system_extensions, 1)
system_ext_define = '#define ' + system_extensions
configh_data.set_quoted('DFLT_XKB_LEGACY_ROOT', XKB_LEGACY_ROOT)
configh_data.set_quoted('DFLT_XKB_CONFIG_ROOT', XKB_CONFIG_ROOT)
if XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH != ''
    configh_data.set_quoted(
        'DFLT_XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH',
        XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH,
    )
endif
if XKB_CONFIG_VERSIONED_EXTENSIONS_PATH != ''
    configh_data.set_quoted(
        'DFLT_XKB_CONFIG_VERSIONED_EXTENSIONS_PATH',
        XKB_CONFIG_VERSIONED_EXTENSIONS_PATH,
    )
endif
configh_data.set_quoted('DFLT_XKB_CONFIG_EXTRA_PATH', XKB_CONFIG_EXTRA_PATH)
configh_data.set_quoted('XLOCALEDIR', XLOCALEDIR)
configh_data.set_quoted('DEFAULT_XKB_RULES', get_option('default-rules'))
configh_data.set_quoted('DEFAULT_XKB_MODEL', get_option('default-model'))
configh_data.set_quoted('DEFAULT_XKB_LAYOUT', get_option('default-layout'))
if get_option('default-variant') != ''
    configh_data.set_quoted('DEFAULT_XKB_VARIANT', get_option('default-variant'))
else
    configh_data.set('DEFAULT_XKB_VARIANT', 'NULL')
endif
if get_option('default-options') != ''
    configh_data.set_quoted('DEFAULT_XKB_OPTIONS', get_option('default-options'))
else
    configh_data.set('DEFAULT_XKB_OPTIONS', 'NULL')
endif
have_unistd_h = cc.has_header('unistd.h')
configh_data.set10('HAVE_UNISTD_H', have_unistd_h)
have_dirent_h = cc.has_header('dirent.h')
configh_data.set10('HAVE_DIRENT_H', have_dirent_h)
has_extensions_directories = have_unistd_h and have_dirent_h
configh_data.set10('HAVE_XKB_EXTENSIONS_DIRECTORIES', has_extensions_directories)
if cc.links(
    'int main(void){if(__builtin_expect(1<0,0)){}}',
    name: '__builtin_expect',
)
    configh_data.set10('HAVE___BUILTIN_EXPECT', true)
endif
# Test if counted_by attribute is available, and supported on pointer fields
# (not just flexible array fields).
if cc.compiles(
    '''
    struct test {
        int count;
        int *items __attribute__((counted_by(count)));
    };
    int main(void) {
        struct test t = {0};
        return 0;
    }
    ''',
    args: cc.get_supported_arguments('-Werror=attributes'),
    name: 'counted_by attribute for pointers',
)
    configh_data.set10('HAVE_ATTR_COUNTED_BY', true)
endif
if cc.has_header_symbol('unistd.h', 'eaccess', prefix: system_ext_define)
    configh_data.set10('HAVE_EACCESS', true)
endif
if cc.has_header_symbol('unistd.h', 'euidaccess', prefix: system_ext_define)
    configh_data.set10('HAVE_EUIDACCESS', true)
endif
if cc.has_header_symbol('sys/mman.h', 'mmap')
    configh_data.set10('HAVE_MMAP', true)
endif
if cc.has_header_symbol('stdlib.h', 'mkostemp', prefix: system_ext_define)
    configh_data.set10('HAVE_MKOSTEMP', true)
endif
if cc.has_header_symbol('fcntl.h', 'posix_fallocate', prefix: system_ext_define)
    configh_data.set10('HAVE_POSIX_FALLOCATE', true)
endif
if cc.has_header_symbol('string.h', 'strndup', prefix: system_ext_define)
    configh_data.set10('HAVE_STRNDUP', true)
endif
if cc.has_header_symbol('stdio.h', 'asprintf', prefix: system_ext_define)
    configh_data.set10('HAVE_ASPRINTF', true)
elif cc.has_header_symbol('stdio.h', 'vasprintf', prefix: system_ext_define)
    configh_data.set10('HAVE_VASPRINTF', true)
endif
if cc.has_header_symbol('stdio.h', 'open_memstream', prefix: system_ext_define)
    configh_data.set10('HAVE_OPEN_MEMSTREAM', true)
endif
if cc.has_header_symbol('stdlib.h', 'secure_getenv', prefix: system_ext_define)
    configh_data.set10('HAVE_SECURE_GETENV', true)
elif cc.has_header_symbol(
    'stdlib.h',
    '__secure_getenv',
    prefix: system_ext_define,
)
    configh_data.set10('HAVE___SECURE_GETENV', true)
else
    message('C library does not support secure_getenv, using getenv instead')
endif
if cc.has_header_symbol('stdlib.h', 'realpath', prefix: system_ext_define)
    configh_data.set10('HAVE_REAL_PATH', true)
endif
if not cc.has_header_symbol('limits.h', 'PATH_MAX', prefix: system_ext_define)
    if host_machine.system() == 'windows'
        # see https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file#maximum-path-length-limitation
        configh_data.set('PATH_MAX', 260)
    else
        configh_data.set('PATH_MAX', 4096)
    endif
endif
if cc.has_header_symbol(
    'time.h',
    'CLOCK_PROCESS_CPUTIME_ID',
    prefix: system_ext_define,
)
    configh_data.set10('HAVE_CLOCK_PROCESS_CPUTIME_ID', true)
endif
if cc.has_header_symbol('time.h', 'CLOCK_MONOTONIC', prefix: system_ext_define)
    configh_data.set10('HAVE_CLOCK_MONOTONIC', true)
endif
if cc.has_header_symbol('termios.h', 'tcsetattr', prefix: system_ext_define)
    configh_data.set10('HAVE_TERMIOS', true)
endif
has_glibc = cc.has_function(
    'gnu_get_libc_version',
    prefix: '#include <gnu/libc-version.h>',
)
has_compose_locale_fallback = false
if cc.has_header_symbol('locale.h', 'newlocale', prefix: system_ext_define)
    # Use newlocale only if it fails on missing locales
    if meson.can_run_host_binaries()
        # Check newlocale implementation
        r = cc.run(
            '''
            #define _GNU_SOURCE
            #include <locale.h>
            int main(void){
                locale_t loc = newlocale(LC_ALL_MASK, "¡NONSENSE!", (locale_t) 0);
                return (loc != (locale_t) 0);
            }''',
            name: 'newlocale fails on invalid locales',
        )
        if r.compiled() and r.returncode() == 0
            has_compose_locale_fallback = true
        endif
    elif has_glibc
        # Cannot check implementation, so enable only for implementations
        # known to failed on missing locales.
        has_compose_locale_fallback = true
    endif
    if has_compose_locale_fallback
        configh_data.set10('HAVE_NEWLOCALE', true)
    endif
endif

# Silence some security & deprecation warnings on MSVC
# for some unix/C functions we use.
# https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4996?view=vs-2019
configh_data.set10('_CRT_SECURE_NO_WARNINGS', true)
configh_data.set10('_CRT_NONSTDC_NO_WARNINGS', true)
configh_data.set10('_CRT_NONSTDC_NO_DEPRECATE', true)
# Reduce unnecessary includes on MSVC.
configh_data.set10('WIN32_LEAN_AND_MEAN', true)

xkbcommon_map = meson.current_source_dir() / 'xkbcommon.map'

# Supports -Wl,--version-script?
meson_test_map = meson.current_source_dir() / 'test' / 'meson_test.map'
have_version_script = cc.links(
    'int main(void) { return 0; }',
    args: f'-Wl,--version-script=@meson_test_map@',
    name: '-Wl,--version-script',
)

map_to_def = find_program('scripts/map-to-def')

# libxkbcommon.
bison = find_program('bison', 'win_bison', required: true, version: '>= 3.6')
yacc = bison
yacc_gen = generator(
    bison,
    output: ['@BASENAME@.c', '@BASENAME@.h'],
    arguments: [
        '--defines=@OUTPUT1@',
        '-o',
        '@OUTPUT0@',
        '-p',
        '_xkbcommon_',
        '@INPUT@',
    ],
)
libxkbcommon_sources = [
    'src/compose/parser.c',
    'src/compose/paths.c',
    'src/compose/state.c',
    'src/compose/table.c',
    'src/xkbcomp/action.c',
    'src/xkbcomp/ast-build.c',
    'src/xkbcomp/compat.c',
    'src/xkbcomp/expr.c',
    'src/xkbcomp/include.c',
    'src/xkbcomp/keycodes.c',
    'src/xkbcomp/keymap.c',
    'src/xkbcomp/keymap-dump.c',
    'src/xkbcomp/keywords.c',
    yacc_gen.process('src/xkbcomp/parser.y'),
    'src/xkbcomp/rules.c',
    'src/xkbcomp/scanner.c',
    'src/xkbcomp/symbols.c',
    'src/xkbcomp/types.c',
    'src/xkbcomp/vmod.c',
    'src/xkbcomp/xkbcomp.c',
    'src/atom.c',
    'src/context.c',
    'src/context-priv.c',
    'src/features.c',
    'src/keysym.c',
    'src/keysym-case-mappings.c',
    'src/keysym-utf.c',
    'src/keymap.c',
    'src/keymap-compare.c',
    'src/keymap-priv.c',
    'src/rmlvo.c',
    'src/scanner-utils.c',
    'src/state.c',
    'src/text.c',
    'src/utf8.c',
    'src/utf8-decoding.c',
    'src/utils.c',
    'src/utils-paths.c',
]
libxkbcommon_link_args = []
libxkbcommon_link_deps = []
if have_version_script
    libxkbcommon_link_args += f'-Wl,--version-script=@xkbcommon_map@'
    libxkbcommon_link_deps += 'xkbcommon.map'
elif cc.get_argument_syntax() == 'msvc'
    libxkbcommon_def = custom_target(
        'xkbcommon.def',
        command: [map_to_def, '@INPUT@', soname_version, '@OUTPUT@'],
        input: 'xkbcommon.map',
        output: 'xkbcommon.def',
    )
    libxkbcommon_link_deps += libxkbcommon_def
    libxkbcommon_link_args += '/DEF:' + libxkbcommon_def.full_path()
endif
libxkbcommon = library(
    'xkbcommon',
    libxkbcommon_sources,
    link_args: libxkbcommon_link_args,
    link_depends: libxkbcommon_link_deps,
    gnu_symbol_visibility: 'hidden',
    version: soname_version,
    install: true,
    include_directories: include_directories('src', 'include'),
)
# Some tests need to use unexported symbols, so we link them against
# an internal copy of libxkbcommon with all symbols exposed.
libxkbcommon_test_internal = library(
    'xkbcommon-test-internal',
    libxkbcommon_sources,
    'src/xkbcomp/keymap-file-iterator.c',
    c_args: ['-DENABLE_PRIVATE_APIS'],
    gnu_symbol_visibility: 'hidden',
    install: false,
    include_directories: include_directories('src', 'include'),
)
install_headers(
    'include/xkbcommon/xkbcommon.h',
    'include/xkbcommon/xkbcommon-compat.h',
    'include/xkbcommon/xkbcommon-compose.h',
    'include/xkbcommon/xkbcommon-errors.h',
    'include/xkbcommon/xkbcommon-features.h',
    'include/xkbcommon/xkbcommon-keysyms.h',
    'include/xkbcommon/xkbcommon-names.h',
    subdir: 'xkbcommon',
)

dep_libxkbcommon = declare_dependency(
    link_with: libxkbcommon,
    include_directories: include_directories('include'),
)
meson.override_dependency('xkbcommon', dep_libxkbcommon)
pkgconfig_variables = [
    f'xkb_root=@XKB_CONFIG_ROOT@',
    f'xkb_extra_path=@XKB_CONFIG_EXTRA_PATH@',
]
if has_extensions_directories
    pkgconfig_variables += [
        f'xkb_unversioned_extensions_path=@XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH@',
        f'xkb_versioned_extensions_path=@XKB_CONFIG_VERSIONED_EXTENSIONS_PATH@',
    ]
endif
pkgconfig.generate(
    libxkbcommon,
    name: 'xkbcommon',
    filebase: 'xkbcommon',
    version: meson.project_version(),
    description: 'XKB API common to servers and clients',
    variables: pkgconfig_variables,
)


# libxkbcommon-x11.
if get_option('enable-x11')
    xcb_dep = dependency('xcb', version: '>=1.10', required: false)
    xcb_xkb_dep = dependency('xcb-xkb', version: '>=1.10', required: false)
    if not xcb_dep.found() or not xcb_xkb_dep.found()
        error(
            '''X11 support requires xcb-xkb >= 1.10 which was not found.
You can disable X11 support with -Denable-x11=false.''',
        )
    endif

    libxkbcommon_x11_sources = files(
        'src/atom.c',
        'src/context-priv.c',
        'src/context.c',
        'src/keymap-priv.c',
        'src/utils.c',
        'src/x11/keymap.c',
        'src/x11/state.c',
        'src/x11/util.c',
    )
    libxkbcommon_x11_link_args = []
    libxkbcommon_x11_link_deps = []
    if have_version_script
        libxkbcommon_x11_link_args += '-Wl,--version-script=' + meson.current_source_dir() / 'xkbcommon-x11.map'
        libxkbcommon_x11_link_deps += 'xkbcommon-x11.map'
    elif cc.get_argument_syntax() == 'msvc'
        libxkbcommon_x11_def = custom_target(
            'xkbcommon-x11.def',
            command: [map_to_def, '@INPUT@', soname_version, '@OUTPUT@'],
            input: 'xkbcommon-x11.map',
            output: 'xkbcommon-x11.def',
        )
        libxkbcommon_x11_link_deps += libxkbcommon_x11_def
        libxkbcommon_x11_link_args += '/DEF:' + libxkbcommon_x11_def.full_path()
    endif
    libxkbcommon_x11 = library(
        'xkbcommon-x11',
        libxkbcommon_x11_sources,
        link_args: libxkbcommon_x11_link_args,
        link_depends: libxkbcommon_x11_link_deps,
        gnu_symbol_visibility: 'hidden',
        version: soname_version,
        install: true,
        include_directories: include_directories('src', 'include'),
        link_with: libxkbcommon,
        dependencies: [
            xcb_dep,
            xcb_xkb_dep,
        ],
    )
    # Some tests need to use unexported symbols, so we link them against
    # an internal copy of libxkbcommon-x11 with all symbols exposed.
    libxkbcommon_x11_test_internal = library(
        'xkbcommon-x11-internal',
        libxkbcommon_x11_sources,
        c_args: ['-DENABLE_PRIVATE_APIS'],
        gnu_symbol_visibility: 'hidden',
        install: false,
        include_directories: include_directories('src', 'include'),
        link_with: libxkbcommon_test_internal,
        dependencies: [
            xcb_dep,
            xcb_xkb_dep,
        ],
    )
    install_headers(
        'include/xkbcommon/xkbcommon-x11.h',
        subdir: 'xkbcommon',
    )
    dep_libxkbcommon_x11 = declare_dependency(
        link_with: libxkbcommon_x11,
        include_directories: include_directories('include'),
    )
    meson.override_dependency('xkbcommon-x11', dep_libxkbcommon_x11)
    pkgconfig.generate(
        libxkbcommon_x11,
        name: 'xkbcommon-x11',
        filebase: 'xkbcommon-x11',
        version: meson.project_version(),
        description: 'XKB API common to servers and clients - X11 support',
        requires: ['xkbcommon'],
        requires_private: ['xcb>=1.10', 'xcb-xkb>=1.10'],
    )
endif

# libxkbregistry
if get_option('enable-xkbregistry')
    dep_libxml = dependency('libxml-2.0')
    # Since libxml-2.13
    if cc.has_header_symbol(
        'libxml/parser.h',
        'xmlCtxtSetErrorHandler',
        prefix: system_ext_define,
        dependencies: dep_libxml,
    )
        configh_data.set10('HAVE_XML_CTXT_SET_ERRORHANDLER', true)
    endif
    # Since libxml-2.14
    if cc.has_header_symbol(
        'libxml/parser.h',
        'xmlCtxtParseDtd',
        prefix: system_ext_define,
        dependencies: dep_libxml,
    )
        configh_data.set10('HAVE_XML_CTXT_PARSE_DTD', true)
    endif
    deps_libxkbregistry = [dep_libxml]
    libxkbregistry_sources = [
        'src/registry.c',
        'src/utils.c',
        'src/util-list.c',
    ]
    libxkbregistry_link_args = []
    libxkbregistry_link_deps = []
    if have_version_script
        libxkbregistry_link_args += '-Wl,--version-script=' + meson.current_source_dir() / 'xkbregistry.map'
        libxkbregistry_link_deps += 'xkbregistry.map'
    elif cc.get_argument_syntax() == 'msvc'
        libxkbregistry_def = custom_target(
            'xkbregistry.def',
            command: [map_to_def, '@INPUT@', soname_version, '@OUTPUT@'],
            input: 'xkbregistry.map',
            output: 'xkbregistry.def',
        )
        libxkbregistry_link_deps += libxkbregistry_def
        libxkbregistry_link_args += '/DEF:' + libxkbregistry_def.full_path()
    endif
    libxkbregistry = library(
        'xkbregistry',
        libxkbregistry_sources,
        link_args: libxkbregistry_link_args,
        link_depends: libxkbregistry_link_deps,
        gnu_symbol_visibility: 'hidden',
        dependencies: deps_libxkbregistry,
        version: soname_version,
        install: true,
        include_directories: include_directories('src', 'include'),
    )
    install_headers(
        'include/xkbcommon/xkbregistry.h',
        subdir: 'xkbcommon',
    )
    pkgconfig.generate(
        libxkbregistry,
        name: 'xkbregistry',
        filebase: 'xkbregistry',
        version: meson.project_version(),
        description: 'XKB API to query available rules, models, layouts, variants and options',
    )

    dep_libxkbregistry = declare_dependency(
        link_with: libxkbregistry,
        include_directories: include_directories('include'),
    )
    meson.override_dependency('xkbregistry', dep_libxkbregistry)
endif

# Tools
icu_dep = dependency('icu-uc', required: false)
tools_shared_lib = static_library(
    'tools-shared',
    sources: [
        'tools/tools-common.c',
        'src/keymap-formats.c',
        'src/utf8-decoding.c',
    ],
    include_directories: include_directories('include', 'src', 'tools'),
)
build_tools = get_option('enable-tools') and cc.has_header_symbol(
    'getopt.h',
    'getopt_long',
    prefix: '#define _GNU_SOURCE',
)
if build_tools
    subdir('tools')
else
    tools_dep = declare_dependency()
endif

# Tests
subdir('test')

# Fuzzing target programs.
subdir('fuzz')

# Benchmarks.
subdir('bench')

# Documentation.
if get_option('enable-docs')
    subdir('doc')
endif

configure_file(output: 'config.h', configuration: configh_data)


# Stable variables for projects using xkbcommon as a subproject.
# These variables should not be renamed.
libxkbcommon_dep = dep_libxkbcommon
if get_option('enable-x11')
    libxkbcommon_x11_dep = dep_libxkbcommon_x11
endif
if get_option('enable-xkbregistry')
    libxkbregistry_dep = dep_libxkbregistry
endif

summary(
    {
        'backend': meson.backend(),
        'buildtype': get_option('buildtype'),
        'id': cc.get_id(),
        'c_args': get_option('c_args'),
        'c_link_args': get_option('c_link_args'),
        'yacc': yacc.full_path() + ' ' + yacc.version(),
        'glibc': has_glibc,
    },
    section: 'Compiler',
)
summary(
    {
        'prefix': get_option('prefix'),
        'bindir': get_option('bindir'),
        'libdir': get_option('libdir'),
        'datadir': get_option('datadir'),
        'xkb-config-root': XKB_CONFIG_ROOT,
        'xkb-legacy-root': XKB_LEGACY_ROOT,
        'xkb-config-unversioned-extensions-path': XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH,
        'xkb-config-versioned-extensions-path': XKB_CONFIG_VERSIONED_EXTENSIONS_PATH,
        'xkb-config-extra-path': XKB_CONFIG_EXTRA_PATH,
        'xlocaledir': XLOCALEDIR,
    },
    section: 'Directories',
)
summary(
    {
        'docs': get_option('enable-docs'),
        'tools': get_option('enable-tools'),
        'wayland': get_option('enable-wayland'),
        'x11': get_option('enable-x11'),
        'compose locale fallback': has_compose_locale_fallback,
        'XKB extensions directories': has_extensions_directories,
    },
    section: 'Features',
)
summary(
    {
        'layout': get_option('default-layout'),
        'model': get_option('default-model'),
        'options': get_option('default-options'),
        'rules': get_option('default-rules'),
        'variant': get_option('default-variant'),
    },
    section: 'Defaults',
)
summary(
    {
        'merge-modes': has_merge_modes_tests,
    },
    section: 'Tests',
)
