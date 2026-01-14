project(
    'libxkbcommon',
    'c',
    version: '1.13.0',
    default_options: [
        'c_std=c11',
        'warning_level=3',
    ],
    meson_version : '>= 1.4.0', # Released on March 2024
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
        ['0', xkbcommon_project_version[1], xkbcommon_project_version[2]]
    )
endif

pkgconfig = import('pkgconfig')
cc = meson.get_compiler('c')

dir_libexec = get_option('prefix')/get_option('libexecdir')/'xkbcommon'

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
    '/wd4100', # Disable MSVC C4100: unreferenced formal parameter
]
add_project_arguments(cc.get_supported_arguments(cflags), language: 'c')

# The legacy X11 config root. Only xkeyboard-config 2.* provides it.
# This is intended when using old xkeyboard-config and as a last resort for
# misconfigured setups.
# Try both un- and versioned pkg-config (xkeyboard-config < 2.45).
XKB_LEGACY_ROOT = ''
foreach v: ['-2', '']
    xkeyboard_config_dep = dependency(f'xkeyboard-config@v@', required: false)
    if xkeyboard_config_dep.found()
        XKB_LEGACY_ROOT = xkeyboard_config_dep.get_variable(
            pkgconfig: 'xkb_base',
            default_value: ''
        )
        break
    endif
endforeach
if XKB_LEGACY_ROOT == ''
    # Fallback to the legacy X11 directory
    XKB_LEGACY_ROOT = get_option('prefix')/get_option('datadir')/'X11'/'xkb'
endif

# The xkeyboard-config root
# Try to set it from the xkeyboard-config package, from the most recent to the
# oldest major version. The hard-coded major versions list should cover a few
# decades, given the expected pace of xkeyboard-config format changes.
XKEYBOARD_CONFIG_ROOT = ''
XKEYBOARD_CONFIG_VERSION_MAJOR = ''
XKEYBOARD_CONFIG_VERSIONED_EXTENSIONS_PATH = ''
XKEYBOARD_CONFIG_UNVERSIONED_EXTENSIONS_PATH = ''
foreach v: [12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2]
    xkeyboard_config_dep = dependency(f'xkeyboard-config-@v@', required: false)
    if xkeyboard_config_dep.found()
        XKEYBOARD_CONFIG_ROOT = xkeyboard_config_dep.get_variable(pkgconfig: 'xkb_root')
        XKEYBOARD_CONFIG_VERSION_MAJOR = xkeyboard_config_dep.version().split('.')[0]
        # The following variables may not be defined
        XKEYBOARD_CONFIG_VERSIONED_EXTENSIONS_PATH = \
            xkeyboard_config_dep.get_variable(
                pkgconfig: 'xkb_versioned_extensions_path',
                default_value: '',
            )
        XKEYBOARD_CONFIG_UNVERSIONED_EXTENSIONS_PATH = \
            xkeyboard_config_dep.get_variable(
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
        XKEYBOARD_CONFIG_ROOT = xkeyboard_config_dep.get_variable(pkgconfig: 'xkb_base')
        XKEYBOARD_CONFIG_VERSION_MAJOR = xkeyboard_config_dep.version().split('.')[0]
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
XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH = get_option('xkb-config-unversioned-extensions-path')
if XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH == '' and XKEYBOARD_CONFIG_ROOT != ''
    if XKEYBOARD_CONFIG_UNVERSIONED_EXTENSIONS_PATH != ''
        XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH = XKEYBOARD_CONFIG_UNVERSIONED_EXTENSIONS_PATH
    elif XKEYBOARD_CONFIG_ROOT.endswith(f'-@XKEYBOARD_CONFIG_VERSION_MAJOR@')
        # The xkeyboard-config root is versioned: remove the version suffix
        # Meson does not have a length() function… Compute the length manually
        s = XKEYBOARD_CONFIG_VERSION_MAJOR
        foreach len: range(3)
            if s == ''
                break
            endif
            s = s.substring(1)
        endforeach
        XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH = XKEYBOARD_CONFIG_ROOT.substring(0, -len - 1) + '.d'
    else
        XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH = get_option('prefix')/get_option('datadir')/'xkeyboard-config.d'
    endif
endif
XKB_CONFIG_VERSIONED_EXTENSIONS_PATH = get_option('xkb-config-versioned-extensions-path')
if XKB_CONFIG_VERSIONED_EXTENSIONS_PATH == '' and XKEYBOARD_CONFIG_ROOT != ''
    if XKEYBOARD_CONFIG_VERSIONED_EXTENSIONS_PATH != ''
        XKB_CONFIG_VERSIONED_EXTENSIONS_PATH = XKEYBOARD_CONFIG_VERSIONED_EXTENSIONS_PATH
    else
        XKB_CONFIG_VERSIONED_EXTENSIONS_PATH = f'@XKEYBOARD_CONFIG_ROOT@.d'
    endif
endif

XKB_CONFIG_EXTRA_PATH = get_option('xkb-config-extra-path')
if XKB_CONFIG_EXTRA_PATH == ''
    XKB_CONFIG_EXTRA_PATH = get_option('prefix')/get_option('sysconfdir')/'xkb'
endif

# The X locale directory for compose.
XLOCALEDIR = get_option('x-locale-root')
if XLOCALEDIR == ''
    XLOCALEDIR = get_option('prefix')/get_option('datadir')/'X11'/'locale'
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
        XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH
    )
endif
if XKB_CONFIG_VERSIONED_EXTENSIONS_PATH != ''
    configh_data.set_quoted(
        'DFLT_XKB_CONFIG_VERSIONED_EXTENSIONS_PATH',
        XKB_CONFIG_VERSIONED_EXTENSIONS_PATH
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
if cc.has_header('unistd.h')
    configh_data.set10('HAVE_UNISTD_H', true)
endif
if cc.has_header('dirent.h')
    configh_data.set10('HAVE_DIRENT_H', true)
endif
if configh_data.get('HAVE_UNISTD_H', 0) == 1 and configh_data.get('HAVE_DIRENT_H', 0) == 1
    configh_data.set10('HAVE_XKB_EXTENSIONS_DIRECTORIES', true)
    has_extensions_directories = true
else
    has_extensions_directories = false
endif
if cc.links('int main(){if(__builtin_expect(1<0,0)){}}', name: '__builtin_expect')
    configh_data.set('HAVE___BUILTIN_EXPECT', 1)
endif
if cc.has_header_symbol('unistd.h', 'eaccess', prefix: system_ext_define)
    configh_data.set('HAVE_EACCESS', 1)
endif
if cc.has_header_symbol('unistd.h', 'euidaccess', prefix: system_ext_define)
    configh_data.set('HAVE_EUIDACCESS', 1)
endif
if cc.has_header_symbol('sys/mman.h', 'mmap')
    configh_data.set('HAVE_MMAP', 1)
endif
if cc.has_header_symbol('stdlib.h', 'mkostemp', prefix: system_ext_define)
    configh_data.set('HAVE_MKOSTEMP', 1)
endif
if cc.has_header_symbol('fcntl.h', 'posix_fallocate', prefix: system_ext_define)
    configh_data.set('HAVE_POSIX_FALLOCATE', 1)
endif
if cc.has_header_symbol('string.h', 'strndup', prefix: system_ext_define)
    configh_data.set('HAVE_STRNDUP', 1)
endif
if cc.has_header_symbol('stdio.h', 'asprintf', prefix: system_ext_define)
    configh_data.set('HAVE_ASPRINTF', 1)
elif cc.has_header_symbol('stdio.h', 'vasprintf', prefix: system_ext_define)
    configh_data.set('HAVE_VASPRINTF', 1)
endif
if cc.has_header_symbol('stdio.h', 'open_memstream', prefix: system_ext_define)
    configh_data.set('HAVE_OPEN_MEMSTREAM', 1)
endif
if cc.has_header_symbol('stdlib.h', 'secure_getenv', prefix: system_ext_define)
    configh_data.set('HAVE_SECURE_GETENV', 1)
elif cc.has_header_symbol('stdlib.h', '__secure_getenv', prefix: system_ext_define)
    configh_data.set('HAVE___SECURE_GETENV', 1)
else
    message('C library does not support secure_getenv, using getenv instead')
endif
if cc.has_header_symbol('stdlib.h', 'realpath', prefix: system_ext_define)
    configh_data.set('HAVE_REAL_PATH', 1)
endif
if not cc.has_header_symbol('limits.h', 'PATH_MAX', prefix: system_ext_define)
    if host_machine.system() == 'windows'
        # see https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file#maximum-path-length-limitation
        configh_data.set('PATH_MAX', 260)
    else
        configh_data.set('PATH_MAX', 4096)
    endif
endif
has_glibc = cc.has_function(
    'gnu_get_libc_version',
    prefix: '#include <gnu/libc-version.h>'
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
        configh_data.set('HAVE_NEWLOCALE', 1)
    endif
endif

# Silence some security & deprecation warnings on MSVC
# for some unix/C functions we use.
# https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4996?view=vs-2019
configh_data.set('_CRT_SECURE_NO_WARNINGS', 1)
configh_data.set('_CRT_NONSTDC_NO_WARNINGS', 1)
configh_data.set('_CRT_NONSTDC_NO_DEPRECATE', 1)
# Reduce unnecessary includes on MSVC.
configh_data.set('WIN32_LEAN_AND_MEAN', 1)

xkbcommon_map = meson.current_source_dir() / 'xkbcommon.map'

# Supports -Wl,--version-script?
meson_test_map = meson.current_source_dir() / 'test/meson_test.map'
have_version_script = cc.links(
    'int main() { return 0; }',
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
    arguments: ['--defines=@OUTPUT1@', '-o', '@OUTPUT0@', '-p', '_xkbcommon_', '@INPUT@'],
)
libxkbcommon_sources = [
    'src/compose/constants.h',
    'src/compose/parser.c',
    'src/compose/parser.h',
    'src/compose/paths.c',
    'src/compose/paths.h',
    'src/compose/state.c',
    'src/compose/table.c',
    'src/compose/table.h',
    'src/xkbcomp/action.c',
    'src/xkbcomp/action.h',
    'src/xkbcomp/ast.h',
    'src/xkbcomp/ast-build.c',
    'src/xkbcomp/ast-build.h',
    'src/xkbcomp/compat.c',
    'src/xkbcomp/expr.c',
    'src/xkbcomp/expr.h',
    'src/xkbcomp/include.c',
    'src/xkbcomp/include.h',
    'src/xkbcomp/keycodes.c',
    'src/xkbcomp/keymap.c',
    'src/xkbcomp/keymap-dump.c',
    'src/xkbcomp/keywords.c',
    yacc_gen.process('src/xkbcomp/parser.y'),
    'src/xkbcomp/parser-priv.h',
    'src/xkbcomp/rules.c',
    'src/xkbcomp/rules.h',
    'src/xkbcomp/scanner.c',
    'src/xkbcomp/symbols.c',
    'src/xkbcomp/types.c',
    'src/xkbcomp/vmod.c',
    'src/xkbcomp/vmod.h',
    'src/xkbcomp/xkbcomp.c',
    'src/xkbcomp/xkbcomp-priv.h',
    'src/atom.c',
    'src/atom.h',
    'src/context.c',
    'src/context.h',
    'src/context-priv.c',
    'src/darray.h',
    'src/keysym.c',
    'src/keysym.h',
    'src/keysym-case-mappings.c',
    'src/keysym-utf.c',
    'src/ks_tables.h',
    'src/keymap.c',
    'src/keymap.h',
    'src/keymap-compare.c',
    'src/keymap-compare.h',
    'src/keymap-priv.c',
    'src/messages-codes.h',
    'src/rmlvo.c',
    'src/rmlvo.h',
    'src/scanner-utils.c',
    'src/scanner-utils.h',
    'src/state.c',
    'src/state-priv.h',
    'src/text.c',
    'src/text.h',
    'src/utf8.c',
    'src/utf8.h',
    'src/utf8-decoding.c',
    'src/utf8-decoding.h',
    'src/utils.c',
    'src/utils.h',
    'src/util-mem.h',
    'src/utils-checked-arithmetic.h',
    'src/utils-numbers.h',
    'src/utils-paths.c',
    'src/utils-paths.h',
]
libxkbcommon_link_args = []
libxkbcommon_link_deps = []
if have_version_script
    libxkbcommon_link_args += f'-Wl,--version-script=@xkbcommon_map@'
    libxkbcommon_link_deps += 'xkbcommon.map'
elif cc.get_argument_syntax() == 'msvc'
    libxkbcommon_def = custom_target('xkbcommon.def',
        command: [map_to_def, '@INPUT@', '@OUTPUT@'],
        input: 'xkbcommon.map',
        output: 'xkbcommon.def',
    )
    libxkbcommon_link_deps += libxkbcommon_def
    libxkbcommon_link_args += '/DEF:' + libxkbcommon_def.full_path()
endif
libxkbcommon = library(
    'xkbcommon',
    'include/xkbcommon/xkbcommon.h',
    libxkbcommon_sources,
    link_args: libxkbcommon_link_args,
    link_depends: libxkbcommon_link_deps,
    gnu_symbol_visibility: 'hidden',
    version: soname_version,
    install: true,
    include_directories: include_directories('src', 'include'),
)
install_headers(
    'include/xkbcommon/xkbcommon.h',
    'include/xkbcommon/xkbcommon-compat.h',
    'include/xkbcommon/xkbcommon-compose.h',
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
        error('''X11 support requires xcb-xkb >= 1.10 which was not found.
You can disable X11 support with -Denable-x11=false.''')
    endif

    libxkbcommon_x11_sources = [
        'src/x11/keymap.c',
        'src/x11/state.c',
        'src/x11/util.c',
        'src/x11/x11-priv.h',
        'src/context.c',
        'src/context.h',
        'src/context-priv.c',
        'src/keymap.h',
        'src/keymap-priv.c',
        'src/atom.h',
        'src/atom.c',
        'src/utils.c',
        'src/utils.h',
    ]
    libxkbcommon_x11_link_args = []
    libxkbcommon_x11_link_deps = []
    if have_version_script
        libxkbcommon_x11_link_args += '-Wl,--version-script=' + meson.current_source_dir()/'xkbcommon-x11.map'
        libxkbcommon_x11_link_deps += 'xkbcommon-x11.map'
    elif cc.get_argument_syntax() == 'msvc'
        libxkbcommon_x11_def = custom_target('xkbcommon-x11.def',
            command: [map_to_def, '@INPUT@', '@OUTPUT@'],
            input: 'xkbcommon-x11.map',
            output: 'xkbcommon-x11.def',
        )
        libxkbcommon_x11_link_deps += libxkbcommon_x11_def
        libxkbcommon_x11_link_args += '/DEF:' + libxkbcommon_x11_def.full_path()
    endif
    libxkbcommon_x11 = library(
        'xkbcommon-x11',
        'include/xkbcommon/xkbcommon-x11.h',
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
    )
        configh_data.set10('HAVE_XML_CTXT_SET_ERRORHANDLER', true)
    endif
    # Since libxml-2.14
    if cc.has_header_symbol(
        'libxml/parser.h',
        'xmlCtxtParseDtd',
        prefix: system_ext_define,
    )
        configh_data.set10('HAVE_XML_CTXT_PARSE_DTD', true)
    endif
    deps_libxkbregistry = [dep_libxml]
    libxkbregistry_sources = [
        'src/registry.c',
        'src/messages-codes.h',
        'src/utils.h',
        'src/utils.c',
        'src/util-list.h',
        'src/util-list.c',
        'src/util-mem.h',
    ]
    libxkbregistry_link_args = []
    libxkbregistry_link_deps = []
    if have_version_script
        libxkbregistry_link_args += '-Wl,--version-script=' + meson.current_source_dir()/'xkbregistry.map'
        libxkbregistry_link_deps += 'xkbregistry.map'
    elif cc.get_argument_syntax() == 'msvc'
        libxkbregistry_def = custom_target('xkbregistry.def',
            command: [map_to_def, '@INPUT@', '@OUTPUT@'],
            input: 'xkbregistry.map',
            output: 'xkbregistry.def',
        )
        libxkbregistry_link_deps += libxkbregistry_def
        libxkbregistry_link_args += '/DEF:' + libxkbregistry_def.full_path()
    endif
    libxkbregistry = library(
        'xkbregistry',
        'include/xkbcommon/xkbregistry.h',
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

man_pages = []

# Tools
icu_dep = dependency('icu-uc', required: false)
build_tools = get_option('enable-tools') and cc.has_header_symbol('getopt.h', 'getopt_long', prefix: '#define _GNU_SOURCE')
if build_tools
    # Common resources
    tools_dep = declare_dependency(
        sources: [
            'tools/tools-common.h',
            'tools/tools-common.c',
            'src/compose/constants.h',
            'src/utf8-decoding.c',
            'src/utf8-decoding.h',
        ],
        include_directories: [include_directories('tools', 'include')],
        dependencies: dep_libxkbcommon,
    )
    configh_data.set10('HAVE_TOOLS', true)

    # Tool: export-keysyms
    export_keysym_dep = [tools_dep]
    if icu_dep.found()
        export_keysym_dep += [icu_dep]
    endif
    executable('export-keysyms',
               'tools/export-keysyms.c',
               libxkbcommon_sources,
               dependencies: export_keysym_dep,
               c_args: ['-DENABLE_PRIVATE_APIS'],
               include_directories: [include_directories('src', 'include')],
               install: false)

    # Tool: xkbcli
    executable('xkbcli', 'tools/xkbcli.c',
               dependencies: tools_dep, install: true)
    install_man('tools/xkbcli.1')

    if get_option('enable-bash-completion')
        bash_completion_path = get_option('bash-completion-path')
        if bash_completion_path == ''
            bash_completion = dependency('bash-completion', required: false)
            if bash_completion.found()
                bash_completion_path = bash_completion.get_variable(pkgconfig: 'completionsdir')
            else
                bash_completion_path = get_option('datadir') / 'bash-completion/completions'
            endif
        endif
        install_data('tools/xkbcli-bash-completion.sh',
                     rename: 'xkbcli',
                     install_dir: bash_completion_path)
    endif

    # Tool: compile-keymap
    xkbcli_compile_keymap = executable('xkbcli-compile-keymap',
                                       'tools/compile-keymap.c',
                                       'src/keymap-formats.c',
                                       'src/keymap-formats.h',
                                       dependencies: tools_dep,
                                       install: true,
                                       install_dir: dir_libexec)
    install_man('tools/xkbcli-compile-keymap.1')
    configh_data.set10('HAVE_XKBCLI_COMPILE_KEYMAP', true)

    # Tool: compose
    executable('xkbcli-compile-compose',
               'tools/compile-compose.c',
               'src/compose/dump.c',
               'src/compose/dump.h',
               'src/compose/escape.h',
               dependencies: tools_dep,
               install: true,
               install_dir: dir_libexec)
    install_man('tools/xkbcli-compile-compose.1')
    configh_data.set10('HAVE_XKBCLI_COMPILE_COMPOSE', true)

    # Tool: how-to-type
    executable('xkbcli-how-to-type',
               'tools/how-to-type.c',
               'src/keymap-formats.c',
               'src/keymap-formats.h',
               dependencies: tools_dep,
               install: true,
               install_dir: dir_libexec)
    install_man('tools/xkbcli-how-to-type.1')
    configh_data.set10('HAVE_XKBCLI_HOW_TO_TYPE', true)

    # Tool: interactive-evdev
    if cc.has_header('linux/input.h')
        executable('xkbcli-interactive-evdev',
                   'tools/interactive-evdev.c',
                   'src/keymap-formats.c',
                   'src/keymap-formats.h',
                   dependencies: tools_dep,
                   install: true,
                   install_dir: dir_libexec)
        configh_data.set10('HAVE_XKBCLI_INTERACTIVE_EVDEV', true)
        install_man('tools/xkbcli-interactive-evdev.1')
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
        executable('xkbcli-interactive-x11',
                   'tools/interactive-x11.c',
                   'src/keymap-formats.c',
                   'src/keymap-formats.h',
                   'src/utils.h',
                   dependencies: x11_tools_dep,
                   install: true,
                   install_dir: dir_libexec)
        install_man('tools/xkbcli-interactive-x11.1')
        configh_data.set10('HAVE_XKBCLI_INTERACTIVE_X11', true)

        executable('xkbcli-dump-keymap-x11',
                   'tools/interactive-x11.c',
                   'src/keymap-formats.c',
                   'src/keymap-formats.h',
                   'src/utils.h',
                   dependencies: x11_tools_dep,
                   c_args: ['-DKEYMAP_DUMP'],
                   install: true,
                   install_dir: dir_libexec)
        install_man('tools/xkbcli-dump-keymap-x11.1')
        configh_data.set10('HAVE_XKBCLI_DUMP_KEYMAP_X11', true)
    endif

    # Tools: interactive-wayland
    if get_option('enable-wayland')
        wayland_client_dep = dependency('wayland-client', version: '>=1.2.0', required: false)
        wayland_protocols_dep = dependency('wayland-protocols', version: '>=1.15', required: false)
        wayland_scanner_dep = dependency('wayland-scanner', required: false, native: true)
        if not wayland_client_dep.found() or not wayland_protocols_dep.found() or not wayland_scanner_dep.found()
            error('''The Wayland xkbcli programs require wayland-client and wayland-protocols which were not found.
You can disable the Wayland xkbcli programs with -Denable-wayland=false.''')
        endif

        wayland_scanner = find_program(wayland_scanner_dep.get_variable(pkgconfig: 'wayland_scanner'))
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
        wayland_protocols_datadir = wayland_protocols_dep.get_variable(pkgconfig: 'pkgdatadir')
        xdg_shell_xml = wayland_protocols_datadir/'stable/xdg-shell/xdg-shell.xml'
        xdg_shell_sources = [
            wayland_scanner_code_gen.process(xdg_shell_xml),
            wayland_scanner_client_header_gen.process(xdg_shell_xml),
        ]
        xdg_decoration_xml = wayland_protocols_datadir/'unstable/xdg-decoration/xdg-decoration-unstable-v1.xml'
        xdg_decoration_sources = [
            wayland_scanner_code_gen.process(xdg_decoration_xml),
            wayland_scanner_client_header_gen.process(xdg_decoration_xml),
        ]
        executable('xkbcli-interactive-wayland',
                   'tools/interactive-wayland.c',
                   'src/keymap-formats.c',
                   'src/keymap-formats.h',
                   'src/utils.h',
                   xdg_shell_sources,
                   xdg_decoration_sources,
                   dependencies: [tools_dep, wayland_client_dep],
                   install: true,
                   install_dir: dir_libexec)
        install_man('tools/xkbcli-interactive-wayland.1')
        configh_data.set10('HAVE_XKBCLI_INTERACTIVE_WAYLAND', true)

        executable('xkbcli-dump-keymap-wayland',
                   'tools/interactive-wayland.c',
                   'src/keymap-formats.c',
                   'src/keymap-formats.h',
                   'src/utils.h',
                   xdg_shell_sources,
                   xdg_decoration_sources,
                   dependencies: [tools_dep, wayland_client_dep],
                   c_args: ['-DKEYMAP_DUMP'],
                   install: true,
                   install_dir: dir_libexec)
        install_man('tools/xkbcli-dump-keymap-wayland.1')
        configh_data.set10('HAVE_XKBCLI_DUMP_KEYMAP_WAYLAND', true)
    endif

    # Tool: interactive
    if (
       configh_data.get('HAVE_XKBCLI_INTERACTIVE_WAYLAND', 0) == 1 or
       configh_data.get('HAVE_XKBCLI_INTERACTIVE_X11', 0) == 1 or
       configh_data.get('HAVE_XKBCLI_INTERACTIVE_EVDEV', 0) == 1
    )
        executable('xkbcli-interactive', 'tools/interactive.c',
                   dependencies: tools_dep,
                   install: true,
                   install_dir: dir_libexec)
    endif

    # Tool: dump-keymap
    if (
       configh_data.get('HAVE_XKBCLI_DUMP_KEYMAP_WAYLAND', 0) == 1 or
       configh_data.get('HAVE_XKBCLI_DUMP_KEYMAP_X11', 0) == 1
    )
        executable('xkbcli-dump-keymap', 'tools/interactive.c',
                   c_args: ['-DKEYMAP_DUMP'],
                   dependencies: tools_dep,
                   install: true,
                   install_dir: dir_libexec)
    endif

    # Tool: list
    if get_option('enable-xkbregistry')
        configh_data.set10('HAVE_XKBCLI_LIST', true)
        executable('xkbcli-list',
                   'tools/registry-list.c',
                   dependencies: dep_libxkbregistry,
                   install: true,
                   install_dir: dir_libexec)
        install_man('tools/xkbcli-list.1')
    endif

    # Tool: check-messages
    executable('xkb-check-messages',
               'tools/check-messages.c',
               'tools/messages.c',
               'tools/messages.h',
               'src/messages-codes.h',
               dependencies: [tools_dep],
               include_directories: [include_directories('src', 'include', 'tools')],
               install: false)
else
    tools_dep = declare_dependency()
endif


# xkeyboard-config "verifier"
xkct_config = configuration_data()
xkct_config.set('MESON_BUILD_ROOT', meson.current_build_dir())
xkct_config.set('XKB_CONFIG_ROOT', XKB_CONFIG_ROOT)
configure_file(input: 'test/xkeyboard-config-test.py.in',
               output: 'xkeyboard-config-test',
               configuration: xkct_config)

# Tests
test_env = environment()
test_env.set('XKB_LOG_LEVEL', 'debug')
test_env.set('XKB_LOG_VERBOSITY', '10')
test_env.set('top_srcdir', meson.current_source_dir())
test_env.set('top_builddir', meson.current_build_dir())
test_env.set('HAVE_XKBCLI_INTERACTIVE_EVDEV', configh_data.get('HAVE_XKBCLI_INTERACTIVE_EVDEV', 0).to_string())
test_env.set('HAVE_XKBCLI_INTERACTIVE_WAYLAND', configh_data.get('HAVE_XKBCLI_INTERACTIVE_WAYLAND', 0).to_string())
test_env.set('HAVE_XKBCLI_INTERACTIVE_X11', configh_data.get('HAVE_XKBCLI_INTERACTIVE_X11', 0).to_string())
test_env.set('HAVE_XKBCLI_LIST', configh_data.get('HAVE_XKBCLI_LIST', 0).to_string())

test_configh_data = configuration_data()
test_configh_data.set_quoted('TEST_XKB_CONFIG_ROOT', meson.current_source_dir()/'test'/'data')
test_configh_data.set('_FORTIFY_SOURCE', '3')
configure_file(output: 'test-config.h', configuration: test_configh_data)

m_dep = cc.find_library('m', required : false)
# Some tests need to use unexported symbols, so we link them against
# an internal copy of libxkbcommon with all symbols exposed.
libxkbcommon_test_internal = library(
    'xkbcommon-test-internal',
    libxkbcommon_sources,
    'src/xkbcomp/keymap-file-iterator.c',
    'src/xkbcomp/keymap-file-iterator.h',
    include_directories: include_directories('src', 'include'),
    c_args: ['-DENABLE_PRIVATE_APIS'],
    gnu_symbol_visibility: 'hidden',
)
test_dep = declare_dependency(
    sources: [
        'bench/bench.c',
        'bench/bench.h',
        'src/utils-numbers.h',
        'src/utf8-decoding.c',
        'src/utf8-decoding.h',
        'test/common.c',
        'test/evdev-scancodes.h',
        'test/test.h',
        'tools/tools-common.c',
        'tools/tools-common.h',
    ],
    include_directories: include_directories('src', 'include'),
    link_with: libxkbcommon_test_internal,
    dependencies: [m_dep],
)

if build_tools
    # Tool: introspection
    executable(
        'introspection',
        'tools/introspection.c',
        'src/utils.c',
        'src/utils.h',
        'src/keymap-formats.c',
        'src/keymap-formats.h',
        'src/xkbcomp/keymap-file-iterator.h',
        dependencies: test_dep,
        include_directories: include_directories('src', 'src/xkbcomp', 'include'),
        install: false
    )

    introspection_config = configuration_data()
    introspection_config.set('MESON_BUILD_ROOT', meson.current_build_dir())
    introspection_config.set('DEFAULT_XKB_RULES', get_option('default-rules'))
    introspection_config.set('DEFAULT_XKB_MODEL', get_option('default-model'))
    introspection_config.set('DEFAULT_XKB_LAYOUT', get_option('default-layout'))
    introspection_config.set('DEFAULT_XKB_VARIANT', get_option('default-variant'))
    introspection_config.set('DEFAULT_XKB_OPTIONS', get_option('default-options'))
    configure_file(
        input: 'tools/introspection-query.py.in',
        output: 'introspection-query',
        configuration: introspection_config
    )
endif

if get_option('enable-x11')
    libxkbcommon_x11_test_internal = library(
        'xkbcommon-x11-internal',
        libxkbcommon_x11_sources,
        include_directories: include_directories('src', 'include'),
        c_args: ['-DENABLE_PRIVATE_APIS'],
        gnu_symbol_visibility: 'hidden',
        link_with: libxkbcommon_test_internal,
        dependencies: [
            xcb_dep,
            xcb_xkb_dep,
        ],
    )
    x11_test_dep = declare_dependency(
        link_with: libxkbcommon_x11_test_internal,
        dependencies: [
            test_dep,
            xcb_dep,
        ],
    )
    x11_xvfb_test_dep = declare_dependency(
        sources: [
            'test/xvfb-wrapper.c',
            'test/xvfb-wrapper.h',
        ],
        dependencies: [x11_test_dep],
    )
endif
# TODO: version range?
keysyms_test_dep = [test_dep]
keysyms_test_c_args = ['-DENABLE_PRIVATE_APIS']
if icu_dep.found()
    keysyms_test_dep += [icu_dep]
    configh_data.set10('HAVE_ICU', true)
endif
test(
    'keysym',
    executable('test-keysym', 'test/keysym.c', 'test/keysym.h',
               dependencies: keysyms_test_dep,
               c_args: keysyms_test_c_args),
    env: test_env,
)
test(
    'keysym-unicode',
    executable('test-keysym-unicode', 'test/keysym-unicode.c',
               dependencies: keysyms_test_dep,
               c_args: keysyms_test_c_args),
    env: test_env,
)
test(
    'keymap',
    executable(
        'test-keymap',
        'test/keymap.c',
        'test/keysym.h',
        'test/keysym-case-mapping.h',
        'src/keymap-formats.c',
        'src/keymap-formats.h',
        dependencies: test_dep),
    env: test_env,
)
test(
    'filecomp',
    executable('test-filecomp', 'test/filecomp.c', dependencies: test_dep),
    env: test_env,
)
test(
    'context',
    executable('test-context', 'test/context.c', dependencies: test_dep),
    env: test_env,
)
test(
    'rules-file',
    executable('test-rules-file', 'test/rules-file.c', dependencies: test_dep),
    env: test_env,
)
test(
    'rules-file-includes',
    executable('test-rules-file-includes', 'test/rules-file-includes.c', dependencies: test_dep),
    env: test_env,
)
test(
    'stringcomp',
    executable(
        'test-stringcomp',
        'test/stringcomp.c',
        'test/utils-text.c',
        'test/utils-text.h',
        dependencies: test_dep
    ),
    env: test_env,
)
test(
    'buffercomp',
    executable('test-buffercomp', 'test/buffercomp.c', dependencies: test_dep),
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
            'test/extensions-directories.c',
            dependencies: extensions_test_deps,
            c_args: extensions_test_c_args,
        ),
        env: test_env,
    )
endif
test(
    'introspection',
    executable(
        'test-introspection',
        'test/introspection.c',
        dependencies: test_dep
    ),
    env: test_env,
)

# The merge modes tests are optional and generated locally on-the-fly
pymod = import('python')
python = pymod.find_installation('python3', modules: ['jinja2'], required: false)
has_merge_modes_tests = python.found() and python.language_version().version_compare('>=3.11')
if has_merge_modes_tests
    script = find_program('scripts/update-merge-modes-tests.py')
    merge_modes_tests = [meson.project_name()]
    # NOTE: The following tests deal with third-party XKB compilers and are
    #       reserved for debugging purposes. Uncomment to enable them.
    # merge_modes_tests += 'xkbcomp' # Xorg XKB compiler
    # merge_modes_tests += 'kbvm'    # Jay compositor XKB compiler
    foreach xkb_compiler: merge_modes_tests
        if xkb_compiler == meson.project_name()
            suffix = ''
            extra_args = []
            extra_source_files = []
        else
            suffix = '-' + xkb_compiler
            extra_args = [xkb_compiler]
            extra_source_files = ['test/external_xkb_compilers.h']
        endif
        filename = f'merge-modes@suffix@.c'
        merge_modes_c = custom_target(
            filename,
            build_by_default: true,
            command: [
                script,
                '--root', meson.project_source_root(),
                '--c-out', '@OUTPUT@',
                '--no-xkb',
            ] + extra_args,
            depend_files: ['test/merge_modes.c.jinja'],
            output: filename,
            capture: false,
            install: false
        )
        test(
            f'merge-modes@suffix@',
            executable(
                f'test-merge-modes@suffix@',
                merge_modes_c,
                extra_source_files,
                dependencies: test_dep,
                include_directories: ['test'],
            ),
            env: test_env,
        )
    endforeach
endif

test(
    'log',
    executable('test-log', 'test/log.c', dependencies: test_dep),
    env: test_env,
)
test(
    'atom',
    executable('test-atom', 'test/atom.c', dependencies: test_dep),
    env: test_env,
)
test(
    'utf8',
    executable(
        'test-utf8',
        'test/utf8.c',
        'src/utf8-decoding.c',
        'src/utf8-decoding.h',
        dependencies: test_dep
    ),
    env: test_env,
)
test(
    'state',
    executable('test-state', 'test/state.c', dependencies: test_dep),
    env: test_env,
)
test(
    'keyseq',
    executable('test-keyseq', 'test/keyseq.c', dependencies: test_dep),
    env: test_env,
)
test(
    'rulescomp',
    executable('test-rulescomp', 'test/rulescomp.c', dependencies: test_dep),
    env: test_env,
)
test(
    'compose',
    executable(
        'test-compose',
        'test/compose.c',
        'test/compose-iter.c',
        'test/compose-iter.h',
        'test/utils-text.c',
        'test/utils-text.h',
        'src/compose/constants.h',
        'src/compose/dump.c',
        'src/compose/dump.h',
        'src/compose/escape.h',
        'src/compose/escape.h',
        'src/compose/parser.h',
        dependencies: test_dep
    ),
    env: test_env,
)
test(
    'utils',
    executable(
        'test-utils',
        'src/utils.h',
        'src/utils-numbers.h',
        'src/utils-paths.h',
        'test/utils.c',
        'test/utils-text.c',
        'test/utils-text.h',
        dependencies: test_dep
    ),
    env: test_env,
)
test(
    'symbols-leak-test',
    find_program('test/symbols-leak-test.py'),
    env: test_env,
    suite: ['python-tests'],
)
test(
    'modifiers',
    executable('test-modifiers', 'test/modifiers.c', dependencies: test_dep),
    env: test_env,
)
test(
    'messages',
    executable(
        'test-messages',
        'test/messages.c',
        'tools/messages.c',
        'tools/messages.h',
        include_directories: include_directories('src', 'include', 'tools'),
        dependencies: test_dep),
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
    test(
        'x11',
        executable('test-x11', 'test/x11.c', dependencies: x11_xvfb_test_dep),
        env: test_env,
        is_parallel : false,
    )
    test(
        'x11comp',
        executable(
            'test-x11comp',
            'test/x11comp.c',
            'test/utils-text.c',
            'test/utils-text.h',
            dependencies: x11_xvfb_test_dep
        ),
        env: test_env,
        is_parallel : false,
    )
endif
if get_option('enable-xkbregistry')
    test(
        'registry',
        executable('test-registry', 'test/registry.c',
                   include_directories: include_directories('src'),
                   dependencies: [dep_libxkbregistry, dep_libxml, test_dep]),
        env: test_env,
    )
endif
if build_tools
    test('tool-option-parsing',
         find_program('test/tool-option-parsing.py'),
         env: test_env,
         suite: ['python-tests'])

    # A set of keysyms to test for. Add one or two symbols to this array
    # whenever the xorgproto gets updated to make sure we resolve them.
    keysyms_to_test = [
        'XF86Macro23',
    ]

    env = environment()
    env.set('XKB_CONFIG_ROOT', meson.current_source_dir()/'test'/'data')
    foreach keysym: keysyms_to_test
        test('keysym-test-@0@'.format(keysym),
             find_program('test/test-keysym.py'),
             env: env,
             args: [keysym, '--tool', xkbcli_compile_keymap],
             suite: ['python-tests'])
    endforeach
endif

valgrind = find_program('valgrind', required: false)
if valgrind.found()
    add_test_setup('valgrind',
        exe_wrapper: [valgrind,
                       '--leak-check=full',
                       '--track-origins=yes',
                       '--gen-suppressions=all',
                       '--error-exitcode=99'],
        # This is used in some tests, to avoid excessive run time.
        env: {'RUNNING_VALGRIND': '1'},
        timeout_multiplier : 10)
else
    message('valgrind not found, disabling valgrind test setup')
endif


# Fuzzing target programs.
executable('fuzz-keymap', 'fuzz/keymap/target.c', dependencies: test_dep)
executable('fuzz-compose', 'fuzz/compose/target.c', dependencies: test_dep)


# Benchmarks.
bench_env = environment()
bench_env.set('top_srcdir', meson.current_source_dir())
benchmark(
    'key-proc',
    executable('bench-key-proc', 'bench/key-proc.c', dependencies: test_dep),
    env: bench_env,
)
benchmark(
    'keysym-case-mappings',
    executable(
        'bench-keysym-case-mappings',
        'bench/keysym-case-mappings.c',
        dependencies: test_dep,
        c_args: ['-DENABLE_PRIVATE_APIS'],
    ),
)
benchmark(
    'rulescomp',
    executable('bench-rulescomp', 'bench/rulescomp.c', dependencies: test_dep),
    env: bench_env,
)
if cc.has_header_symbol('getopt.h', 'getopt_long', prefix: '#define _GNU_SOURCE')
    benchmark(
        'rules',
        executable('bench-rules', 'bench/rules.c', dependencies: test_dep),
        env: bench_env,
    )
    benchmark(
        'compile-keymap',
        executable(
            'bench-compile-keymap',
            'bench/compile-keymap.c',
            'src/keymap-formats.c',
            'src/keymap-formats.h',
            'src/utils.c',
            'src/utils.h',
            dependencies: test_dep
        ),
        env: bench_env,
    )
    benchmark(
        'dump-keymap',
        executable(
            'bench-dump-keymap',
            'bench/compile-keymap.c',
            'src/keymap-formats.c',
            'src/keymap-formats.h',
            dependencies: test_dep,
            c_args: ['-DKEYMAP_DUMP'],
        ),
        env: bench_env,
    )
    benchmark(
        'custom-parsers',
        executable(
            'bench-custom-parsers',
            'bench/custom-parsers.c',
            'src/utils.c',
            'src/utils.h',
            'src/utils-numbers.h',
            dependencies: test_dep,
        ),
        env: bench_env,
    )
endif
benchmark(
    'compose',
    executable('bench-compose', 'bench/compose.c', dependencies: test_dep),
    env: bench_env,
)
benchmark(
    'compose-traversal',
    executable(
        'bench-compose-traversal',
        'bench/compose-traversal.c',
        'bench/bench.h',
        'src/compose/constants.h',
        'test/compose-iter.c',
        'test/compose-iter.h',
        'test/test.h',
        dependencies: test_dep
    ),
    env: bench_env,
)
benchmark(
    'atom',
    executable('bench-atom', 'bench/atom.c', dependencies: test_dep),
    env: bench_env,
)
if get_option('enable-x11')
  benchmark(
      'x11',
      executable('bench-x11', 'bench/x11.c', dependencies: x11_test_dep),
      env: bench_env,
  )
endif


# Documentation.
if get_option('enable-docs')
    doxygen = find_program('doxygen', required: false)
    if not doxygen.found()
        error('''Documentation requires doxygen which was not found.
You can disable the documentation with -Denable-docs=false.''')
    endif
    doxygen_wrapper = find_program('scripts/doxygen-wrapper')

    doxygen_input = [
        'README.md',
        'LICENSE',
        'NEWS.md',
        'meson_options.txt',
        'doc/debugging.md',
        'doc/diagrams/xkb-configuration.dot',
        'doc/diagrams/xkb-keymap-components.dot',
        'doc/diagrams/xkb-types-explanation.dot',
        'doc/doxygen-extra.css',
        'doc/faq.md',
        'doc/introduction-to-xkb.md',
        'doc/quick-guide.md',
        'doc/compatibility.md',
        'doc/packaging-keyboard-layouts.md',
        'doc/user-configuration.md',
        'doc/rules-format.md',
        'doc/keymap-text-format-v1-v2.md',
        'doc/message-registry.md',
        'doc/release-notes.md',
        'doc/license.md',
        'include/xkbcommon/xkbcommon.h',
        'include/xkbcommon/xkbcommon-compose.h',
        'include/xkbcommon/xkbcommon-keysyms.h',
        'include/xkbcommon/xkbcommon-names.h',
        'include/xkbcommon/xkbcommon-x11.h',
        'include/xkbcommon/xkbregistry.h',
    ]
    doxygen_data = configuration_data()
    doxygen_data.set('PACKAGE_NAME', meson.project_name())
    doxygen_data.set('PACKAGE_VERSION', meson.project_version())
    doxygen_data.set('INPUT', ' '.join(doxygen_input))
    doxygen_data.set('OUTPUT_DIRECTORY', meson.current_build_dir())
    doxyfile = configure_file(
        input: 'doc/Doxyfile.in',
        output: 'Doxyfile',
        configuration: doxygen_data,
    )
    # TODO: Meson should provide this.
    docdir = get_option('datadir')/'doc'/meson.project_name()
    doc_gen = custom_target(
        'doc',
        input: [doxyfile] + doxygen_input,
        output: 'html',
        command: [
            doxygen_wrapper,
            doxygen,
            meson.current_build_dir()/'Doxyfile',
            meson.current_source_dir(),
        ],
        install: true,
        install_dir: docdir,
        build_by_default: true,
    )
    if get_option('enable-cool-uris')
        ensure_stable_urls = find_program('scripts'/'ensure-stable-doc-urls.py')
        custom_target(
            'doc-cool-uris',
            input: [doc_gen, 'doc'/'cool-uris.yaml'],
            output: 'html-xtra',
            command: [
                ensure_stable_urls,
                'generate-redirections',
                meson.current_source_dir()/'doc'/'cool-uris.yaml',
                meson.current_build_dir()/'html'
            ],
            install: false,
            build_by_default: true,
        )
    endif
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

summary({
    'backend': meson.backend(),
    'buildtype': get_option('buildtype'),
    'id': cc.get_id(),
    'c_args': get_option('c_args'),
    'c_link_args': get_option('c_link_args'),
    'yacc': yacc.full_path() + ' ' + yacc.version(),
    'glibc': has_glibc,
}, section: 'Compiler')
summary({
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
}, section: 'Directories')
summary({
    'docs': get_option('enable-docs'),
    'tools': get_option('enable-tools'),
    'wayland': get_option('enable-wayland'),
    'x11': get_option('enable-x11'),
    'compose locale fallback': has_compose_locale_fallback,
    'XKB extensions directories': has_extensions_directories,
}, section: 'Features')
summary({
    'layout': get_option('default-layout'),
    'model': get_option('default-model'),
    'options': get_option('default-options'),
    'rules': get_option('default-rules'),
    'variant': get_option('default-variant'),
}, section: 'Defaults')
summary({
    'merge-modes': has_merge_modes_tests,
}, section: 'Tests')
