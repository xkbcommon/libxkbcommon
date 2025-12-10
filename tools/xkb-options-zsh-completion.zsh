#compdef -value-,XKB_DEFAULT_OPTIONS,-default-

local options_yaml
options_yaml="$(_call_program -l xkb-yaml xkbcli list)"

case $status in
	127 ) _message "xkb option completion requires xkbcli" && return 127 ;;
	<1->) _message "error listing xkb options" && return 1 ;;
esac

local yq_prog='.option_groups[] as $grp | $grp.options[].name | sub(":.*", "") as $name | "*\($name)[\($grp.description)]: :->option-\($name)"'
local -Ua option_groups
option_groups=( ${(@f)"$(printf '%s\n' $options_yaml | _call_program -l xkb-parse-yaml yq ${(q)yq_prog})"} )

case $status in
	127 )
        # fallback with sed
		local sed_options='/^option_groups:/,/^$/p'
		local sed_names="s/  - name: '([^']+)'/\\1/p" sed_descs="s/    description: '([^']+)'/\\1/p"
		local options_sed="$(printf '%s\n' $options_yaml | _call_program -l sed sed -n ${(q)sed_options})"
		local -a options_names=( ${(@f)"$(_call_program -l sed-names sed -En ${(q)sed_names} <<< $options_sed)"})
		local -a options_descs=( ${(@f)"$(_call_program -l sed-desc sed -En ${(q)sed_descs} <<< $options_sed)"})
		local -a xkb_options
		printf -v xkb_options '%s:%s' ${${options_names:s/:/\\:}:^options_descs}
		_describe -t xkb-option 'xkb option' xkb_options
		return
		;;
	<1->) _message "error completing xkb options" && return 1 ;;
esac

local context state state_descr line
typeset -A val_args

_values -s, -S: 'xkb option' ${option_groups}
if [[ $state == option-* ]]; then
	local optname=${state#option-}
	local -a xkb_options
	yq_prog='.option_groups[].options[] | select(.name | test("'$optname'" + ":")) | "\(.name):\(.description)"'
	xkb_options=( ${${(@f)"$(printf '%s\n' $options_yaml | _call_program -l xkb-parse-yaml yq ${(q)yq_prog})"}#*:} )
	_describe -t xkb-option 'xkb option' xkb_options -qS,
fi
