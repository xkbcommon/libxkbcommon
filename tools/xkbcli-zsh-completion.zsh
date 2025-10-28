#compdef xkbcli -P xkbcli-*

local context state state_descr line
typeset -A opt_args
local expl

_xkbcli_commands() {
	local -a commands=(
		'list:list available rules, models, layouts, variants and options'
		'interactive:interactive debugger for XKB keymaps'
		'interactive-wayland:interactive debugger for XKB keymaps for Wayland'
		'interactive-x11:interactive debugger for XKB keymaps for X11'
		'interactive-evdev:interactive debugger for XKB keymaps for evdev'
		'dump-keymap:dump a XKB keymap from a Wayland or X11 compositor'
		'dump-keymap-wayland:dump a XKB keymap from a Wayland compositor'
		'dump-keymap-x11:dump a XKB keymap from an X server'
		'compile-keymap:compile an XKB keymap'
		'compile-compose:compile a Compose file'
		'how-to-type:print key sequences to type a Unicode codepoint'
	)

	_describe -t xkbcli-command 'xkbcli command' commands
}

local -a rmlvo_opts_common=(
	'--rules=[the XKB ruleset]:rules:(base evdev)'
	'--model=[the XKB model]:model:_xkb_model'
	'--layout=[the XKB layout]:layout:_xkb_layout'
	'--variant=[the XKB variant]:variant:_xkb_variant'
	'--options=[the XKB options]:options:_xkb_options'
	'--enable-environment-names[set the default RMLVO values from the environment]'
)

_xkbcli-list() {
	_arguments -S : \
		'(-v --verbose)'{-v,--verbose}'[increase verbosity]' \
		'--help[print a help message and exit]' \
		'--ruleset=[load a ruleset]' \
		'--skip-default-paths[do not load the default XKB paths]' \
		'--load-exotic[load the exotic (extra) rulesets]' \
		'*:xkb base directory:_files -/'
}

_xkbcli-interactive() {
	_arguments -S : \
		'--verbose[enable verbose debugging output]' \
		'--help[print a help message and exit]' \
		'(-1 --uniline --multiline)--multiline[enable multiline event output]' \
		'(-1 --uniline --multiline)'{-1,--uniline}'[enable uniline event output]' \
		'(--local-state)--keymap=[use the given keymap instead of the compositor keymap]:keymap:_files' \
		'--local-state[enable local state handling and ignore modifiers/layouts]' \
		'--enable-compose[enable Compose]' \
		'--format=[use the given keymap format]:xkb format:(v1 v2)'
}

_xkbcli-interactive-evdev() {
	_arguments -S : \
		'--help[print a help message and exit]' \
		'--format=[use the given keymap format]:xkb format:(v1 v2)' \
		'--verbose[enable verbose debugging output]' \
		'(-1 --uniline --multiline)'{-1,--uniline}'[enable uniline event output]' \
		'(-1 --uniline --multiline)--multiline[enable multiline event output]' \
		'--short[shorter event output]' \
		'--report-state-changes[report changes to the state]' \
		'--enable-compose[enable Compose]' \
		'--consumed-mode=[select the consumed modifiers mode]:mode:(xkb|gtk)' \
		'--without-x11-offset[do not add X11 keycode offset]' \
		"$rmlvo_opts_common[@]"
}

_xkbcli-dump-keymap() {
	_arguments -S : \
		'--verbose[enable verbose debugging output]' \
		'--help[print a help message and exit]' \
		'(--format)--input-format=[use the given input keymap format]:xkb format:(v1 v2)' \
		'(--format)--output-format=[use the given output keymap format]:xkb format:(v1 v2)' \
		'(--input-format --output-format)--format=[use the given keymap format for input and output]:xkb format:(v1 v2)' \
		'--no-pretty[do not pretty preint when serializing a keymap]' \
		'--drop-unused[disable unused bits serialization]' \
		'--raw[dump raw keymap without parsing it]'
}


_xkbcli-compile-keymap() {
	_arguments -S : \
		'--help[print a help message and exit]' \
		'--verbose[enable verbose debugging output]' \
		'--test[test compilation but do not print the keymap]' \
		+ input \
		'*--include[add the given path to the include path list]' \
		'--include-defaults[add the default set of include directories]' \
		'(--format)--input-format=[the keymap format to use for parsing]:xkb format:(v1 v2)' \
		'(--format)--output-format=[the keymap format to use for serializing]:xkb format:(v1 v2)' \
		'(--input-format --output-format)--format=[the keymap format to use for parsing and serializing]:xkb format:(v1 v2)' \
		'--no-pretty[do not pretty preint when serializing a keymap]' \
		'--drop-unused[disable unused bits serialization]' \
		'(--enable-environment-names)--keymap=[use the given keymap file]:keymap:_files' \
		"$rmlvo_opts_common[@]" \
		+ '(output)' \
		'--kccgst[print a keymap in KcCGST format]' \
		'--kccgst-yaml[print a KcCGST keymap in YAML format]' \
		'--rmlvo[print the full RMLVO in YAML format]' \
		'--modmaps[print the real and virtual key modmaps in YAML format]'
}

_xkbcli-compile-compose() {
	_arguments -S : \
		'--help[print a help message and exit]' \
		'--verbose[enable verbose debugging output]' \
		'--test[test compilation but do not print the Compose file]' \
		'--locale=[use the specified locale]:locale:_locales'
}

_xkbcli-keysyms() {
	local include="/usr/include/xkbcommon/xkbcommon-keysyms.h"
	local -Ua keysyms=(${(@f)"$(_call_program -l keysym grep -Pwo 'XKB_KEY_\\K\\w+' $include)"})
	_wanted keysym expl 'keysym' compadd -a "$@" - keysyms
}

_xkbcli-how-to-type() {
	local context state state_descr line
	typeset -A opt_args

	local ret=1
	_arguments -S : \
		'--help[print a help message and exit]' \
		'--verbose[enable verbose debugging output]' \
		'--keysym[treat the argument only as a keysym]' \
		'--disable-compose[disable Compose support]' \
		+ xkb \
		'--format=[the keymap format to use]' \
		'--keymap=[the keymap file to load]:xkb keymap:_files' \
		"$rmlvo_opts_common[@]" \
		': :->key' && ret=0

		case $state in
			key)
				if (( $+opt_args[--keysym] )); then
					_xkbcli-keysyms && ret=0
				else
					_alternative \
						'character:character:()' \
						'codepoint:codepoint:()' \
						'keysym:keysym:_xkbcli-keysyms' && ret=0
				fi
				;;
		esac
		return ret
}

local ret=1
case $service in
	xkbcli-interactive-(wayland|x11))
		_xkbcli-interactive
		return
		;;
	xkbcli-dump-keymap-(wayland|x11))
		_xkbcli-dump-keymap
		return
		;;
	xkbcli-*)
		_call_function ret "_$service"
		return ret
		;;
	xkbcli)
		_arguments -S : \
			'(-)'{-h,--help}'[show a help message and exit]' \
			'(-)'{-V,--version}'[show version info and exit]' \
			'(-): :->command' \
			'(-)*:: :->option-or-argument' && ret=0
		;;
esac

case $state in
	command)
		_xkbcli_commands && ret=0
		;;
	option-or-argument)
        local curcontext=${curcontext%:*:*}:xkbcli-$words[1]:
		case $words[1] in
			list)
				_xkbcli-list && ret=0 ;;
			interactive(|-wayland|-x11))
				_xkbcli-interactive && ret=0 ;;
			interactive-evdev)
				_xkbcli-interactive-evdev && ret=0 ;;
			dump-keymap(|-wayland|-x11))
				_xkbcli-dump-keymap && ret=0 ;;
			compile-keymap)
				_xkbcli-compile-keymap && ret=0 ;;
			compile-compose)
				_xkbcli-compile-compose && ret=0 ;;
			how-to-type)
				_xkbcli-how-to-type && ret=0 ;;
			*)
				_message "unknown $service command ${(qq)words[1]}"
				;;
		esac
		;;
esac
return ret
