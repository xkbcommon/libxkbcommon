#compdef xkbcli -P xkbcli-*

local context state state_descr line
typeset -A opt_args
local expl

local -a modifiers=(
	"Shift"
	"Lock"
	"Control"
	"Mod1"
	"Mod2"
	"Mod3"
	"Mod4"
	"Mod5"
	"Alt"
	"Hyper"
	"LevelThree"
	"LevelFive"
	"Meta"
	"NumLock"
	"ScrollLock"
	"Super"
)

_xkbcli_modifiers_mapping() {
	local expl
	compset -P '*,'
	# TODO: don't complete duplicate mods in map?
	if compset -P '*:'; then
		_wanted modifier expl "modifier" compadd "$@" -S, -q -a - modifiers
	else
		compset -P '*+'
		_wanted modifier expl "modifier" compadd "$@" -S+ -r "+:\-" -a - modifiers
	fi
}

_xkbcli_modifiers_mask() {
	_values -s+ "modifiers" $modifiers
}

_xkbcli_layout_mapping() {
	compset -P '*,'
	_numbers
}

_xkbcli_controls() {
	local -a pm=(
		'+:Enable a control'
		'-:Disable a control'
	)
	local -a controls=(
		"sticky-keys:sticky-keys accessibility feature"
		"latch-to-lock:latch-to-lock option for sticky-keys"
		"latch-simulations:relax the strict tapping sequence requirement for operating key latches"
	)
	local -a completed_controls=( ${${(s-,-)${words[CURRENT]#*=}}} )
	local -a remaining_controls=( ${controls:#(${(~j.|.)completed_controls#[+-]}):*} )
	(( $#remaining_controls )) || return 0;

	compset -P '*,'
	if compset -P '[+-]'; then
		_describe "xkb controls" remaining_controls -S "," -q
	else
		_alternative \
			'action:action:{_describe "action" pm -S ""}' \
			'xkb-control:xkb controls:{_describe "xkb controls" remaining_controls -S "," -q}'
	fi
}

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

local -a interactive_common=(
	'--help[print a help message and exit]'
	'--verbose[enable verbose debugging output]'
	'(-1 --uniline --multiline)--multiline[enable multiline event output]'
	'(-1 --uniline --multiline)'{-1,--uniline}'[enable uniline event output]'
	'--consumed-mode=[select the consumed modifiers mode]:mode:(xkb gtk)'
	'(--report-state-changes)--no-state-report[do not report changes to the state]'
	'--format=[use the given keymap format]:xkb format:(v1 v2)'
	'--enable-compose[enable Compose]'
	'--legacy-state-api=[use the legacy state API instead of state event API]:state:(true false)'
	'(--local-state --legacy-state-api)--modifiers-mapping=[use the given modifiers mapping]:modifiers mapping:_xkbcli_modifiers_mapping' \
	'(--local-state --legacy-state-api)--shortcuts-mask=[modifier mask to enable selecting a specific layout]:modifiers mask:_xkbcli_modifiers_mask' \
	'(--local-state --legacy-state-api)--shortcuts-mapping=[layout indices mapping to enable selective a specific layout]:layout mapping:_xkbcli_layout_mapping' \
	'(--local-state --legacy-state-api)--controls=[enable or disable controls]:xkb conbtrols:_xkbcli_controls' \
)

_xkbcli_keymap() {
	_alternative \
		'stdin:stdin:((-:"read from stdin"))' \
		'files:keymap file:_files'
}

_xkbcli-interactive() {
	_arguments -S : \
		'--local-state[enable local state handling and ignore modifiers/layouts]' \
		'(--local-state)--keymap=[use the given keymap instead of the compositor keymap]:keymap:_xkbcli_keymap' \
		"$interactive_common[@]"
}

_xkbcli-interactive-evdev() {
	_arguments -S : \
		'--short[shorter event output]' \
		'(--no-state-report)--report-state-changes[report changes to the state]' \
		'--without-x11-offset[do not add X11 keycode offset]' \
		'--keymap=[use the given keymap]:keymap:_xkbcli_keymap' \
		"$interactive_common[@]" \
		"$rmlvo_opts_common[@]"
}

local -a dump_common=(
	'--verbose[enable verbose debugging output]'
	'--help[print a help message and exit]'
	'(--format)--input-format=[use the given input keymap format]:xkb format:(v1 v2)'
	'(--format)--output-format=[use the given output keymap format]:xkb format:(v1 v2)'
	'(--input-format --output-format)--format=[use the given keymap format for input and output]:xkb format:(v1 v2)'
	'--no-pretty[do not pretty preint when serializing a keymap]'
	'--drop-unused[disable unused bits serialization]'
)

_xkbcli-dump-keymap() {
	_arguments -S : \
		"$dump_common[@]"
}

_xkbcli-dump-keymap-wayland() {
	_arguments -S : \
		'--raw[dump raw keymap without parsing it]' \
		"$dump_common[@]"
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
		'--no-pretty[do not pretty print when serializing a keymap]' \
		'--drop-unused[disable unused bits serialization]' \
		'(--enable-environment-names)--keymap=[use the given keymap file]:keymap:_xkbcli_keymap' \
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
		'--format=[the keymap format to use]:xkb format:(v1 v2)' \
		'--keymap=[the keymap file to load]:xkb keymap:_xkbcli_keymap' \
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
	xkbcli-dump-keymap-x11)
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
			dump-keymap(|-x11))
				_xkbcli-dump-keymap && ret=0 ;;
			dump-keymap-wayland)
				_xkbcli-dump-keymap-wayland && ret=0 ;;
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
