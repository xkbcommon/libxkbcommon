doxygen = find_program('doxygen', required: false)
if not doxygen.found()
    error(
        '''Documentation requires doxygen which was not found.
You can disable the documentation with -Denable-docs=false.''',
    )
endif

doxygen_wrapper = find_program(
    meson.project_source_root() / 'scripts' / 'doxygen-wrapper',
)

# NOTE: file paths relative to the project root, which is used as the working
#       directory for Doxygen. This is needed for various reasons (e.g. relative
#       references in md files).
doxygen_input = [
    'README.md',
    'LICENSE',
    'NEWS.md',
    'meson.options',
    'doc/debugging.md',
    'doc/diagrams/xkb-configuration.dot',
    'doc/diagrams/xkb-keymap-components.dot',
    'doc/diagrams/xkb-types-explanation.dot',
    'doc/img/numeric-keypad-overlay.svg',
    'doc/doxygen-extra.css',
    'doc/faq.md',
    'doc/introduction-to-xkb.md',
    'doc/quick-guide.md',
    'doc/compatibility.md',
    'doc/packaging-keyboard-layouts.md',
    'doc/custom-configuration.md',
    'doc/rules-format.md',
    'doc/keymap-text-format-v1-v2.md',
    'doc/message-registry.md',
    'doc/release-notes.md',
    'doc/license.md',
    'include/xkbcommon/xkbcommon.h',
    'include/xkbcommon/xkbcommon-compose.h',
    'include/xkbcommon/xkbcommon-errors.h',
    'include/xkbcommon/xkbcommon-features.h',
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
    input: 'Doxyfile.in',
    output: 'Doxyfile',
    configuration: doxygen_data,
)

relative_doxygen_input = []
foreach f : doxygen_input
    relative_doxygen_input += '..' / f
endforeach

# TODO: Meson should provide this.
docdir = get_option('datadir') / 'doc' / meson.project_name()
doc_gen = custom_target(
    'doxygen',
    input: [doxyfile] + relative_doxygen_input,
    output: 'html',
    command: [
        doxygen_wrapper,
        doxygen,
        meson.current_build_dir() / 'Doxyfile',
        meson.project_source_root(),
    ],
    install: true,
    install_dir: docdir,
    build_by_default: true,
)

if get_option('enable-cool-uris')
    ensure_stable_urls = find_program(
        meson.project_source_root() / 'scripts' / 'ensure-stable-doc-urls.py',
    )
    custom_target(
        'cool-uris',
        input: [doc_gen, meson.current_source_dir() / 'cool-uris.yaml'],
        output: 'html-xtra',
        command: [
            ensure_stable_urls,
            'generate-redirections',
            meson.current_source_dir() / 'cool-uris.yaml',
            meson.current_build_dir() / 'html',
        ],
        install: false,
        build_by_default: true,
    )
endif
