#compdef -value-,XKB_DEFAULT_LAYOUT,-default-

local layouts_yaml
layouts_yaml="$(_call_program -l xkb-yaml xkbcli list)"

case $status in
	127 ) _message "xkb layout completion requires xkbcli" && return 127 ;;
	<1->) _message "error listing xkb layouts" && return 1 ;;
esac

local yq_prog='.layouts[] | select(.variant == "") | "\(.layout):\(.description)"'
local -a xkb_layouts
xkb_layouts=( ${(@f)"$(printf '%s\n' $layouts_yaml | _call_program -l xkb-parse-yaml yq ${(q)yq_prog})"} )

case $status in
	127)
		local sed_layouts='/^layouts:/,/^$/p'
		local sed_names="s/- layout: '([^']+)'/\\1/p" sed_variant="s/  variant: '([^']*)'/v\1/p" sed_descs="s/  description: //p"
		local layouts_sed="$(_call_program -l sed sed -n ${(q)sed_layouts} <<< $layouts_yaml)"
		local -a layout_names=( ${(@f)"$(_call_program -l sed-names sed -En ${(q)sed_names} <<< $layouts_sed)"} )
		local -a layout_variants=( ${(@f)"$(_call_program -l sed-variant sed -En ${(q)sed_variant} <<< $layouts_sed)"} )
		local -a layout_descs=( ${(@f)"$(_call_program -l sed-descs sed -n ${(q)sed_descs} <<< $layouts_sed)"} )
		local -a xkb_layouts1 xkb_layouts2
		printf -v xkb_layouts1 '%s:%s' ${layout_names:^layout_variants}
		printf -v xkb_layouts2 '%s:%s' ${xkb_layouts1:^layout_descs}
		xkb_layouts=( ${${(M)xkb_layouts2:#*:v:*}:s/:v:/:} )
		;;
	<1->) _message "error completing xkb layouts" && return 1 ;;
esac

_describe -t xkb-layout 'xkb layout' xkb_layouts "$@"
