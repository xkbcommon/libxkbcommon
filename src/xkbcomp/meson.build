# Manually regenerate the XKB parser
#
# Run the yacc executable from the project root so that the #line
# references are relative.
fs = import('fs')
yacc_wrapper = find_program(
    meson.project_source_root() / 'scripts' / 'yacc-wrapper',
)
parser_source = fs.relative_to(
    files('parser.y'),
    meson.project_source_root(),
)
parser_implementation = fs.replace_suffix(parser_source, '.c')
parser_header = fs.replace_suffix(parser_implementation, '.h')
custom_target(
    'regenerate',
    input: fs.name(parser_source),
    output: [fs.name(parser_implementation), fs.name(parser_header)],
    command: [
        yacc_wrapper,
        yacc,
        parser_source,
        parser_implementation,
        parser_header,
        meson.project_source_root(),
    ],
    build_by_default: false,
    install: false,
)
