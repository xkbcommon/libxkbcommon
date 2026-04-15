# Regenerate the XKB parser

fs = import('fs')
yacc_wrapper = host_machine.system() == 'windows' ? 'yacc-wrapper.bat' : 'yacc-wrapper'
yacc_wrapper = find_program(
    meson.project_source_root() / 'scripts' / yacc_wrapper,
)

# Paths relative to the project root so that the #line references are relative.
parser_source = fs.relative_to(
    files('parser.y'),
    meson.project_source_root(),
)
parser_implementation = fs.replace_suffix(parser_source, '.c')
parser_header = fs.replace_suffix(parser_implementation, '.h')

if yacc.found()
    # Regenerate manually the parser files in the *source* tree
    parser_target_configs = [
        {
            'target': 'update-parser',
            'parser_implementation': parser_implementation,
            'parser_header': parser_header,
            'build_by_default': true,
        },
    ]
else
    # No regeneration
    parser_target_configs = []
endif

if enable_parser_generation
    # Regenerate automatically the parser files in the *build* directory
    parser_target_configs += {
        'target': 'auto-generate-parser',
        'parser_implementation': meson.current_build_dir() / '_' + fs.name(
            parser_implementation,
        ),
        'parser_header': meson.current_build_dir() / '_' + fs.name(
            parser_header,
        ),
        'build_by_default': true,
    }
    # Will be set in the following foreach loop
    parser_files = []
else
    parser_files = files(fs.name(parser_implementation), fs.name(parser_header))
endif

foreach p : parser_target_configs
    t = custom_target(
        p['target'],
        input: fs.name(parser_source),
        output: [
            fs.name(p['parser_implementation']),
            fs.name(p['parser_header']),
        ],
        depend_files: files(fs.name(parser_source)),
        command: [
            yacc_wrapper,
            yacc,
            parser_source,
            p['parser_implementation'],
            p['parser_header'],
            meson.project_source_root(),
        ],
        build_by_default: p['build_by_default'],
        install: false,
    ).to_list()
    if p['build_by_default']
        parser_files = t
    endif
endforeach
