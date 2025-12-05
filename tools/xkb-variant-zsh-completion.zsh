#compdef -value-,XKB_DEFAULT_VARIANT,-default-

local variants_yaml
variants_yaml="$(_call_program -l xkb-yaml xkbcli list)"

case $status in
	127 ) _message "xkb model completion requires xkbcli" && return 127 ;;
	<1->) _message "error listing xkb variants" && return 1 ;;
esac

local yq_prog1 yq_prog2 layout=${(v)opt_args[(I)*layout]}
if [[ ${(t)opt_args} == *association* && -n $layout ]]; then
	# If we're called from _arguments this will attempt to match the layout
	# given in any --layout option previously seen on the command line
	yq_prog1='.layouts[] | select(.variant != "" and .layout == '${(qqq)${(Q)layout}}') | "\(.layout)(\(.variant)):\(.description)"'
	yq_prog2='.layouts[] | select(.variant != "" and .layout == '${(qqq)${(Q)layout}}') | "\(.variant):\(.description)"'
else
	yq_prog1='.layouts[] | select(.variant != "") | "\(.layout)(\(.variant)):\(.description)"'
	yq_prog2='.layouts[] | select(.variant != "") | "\(.variant):\(.description)"'
fi

local -a variants1 variants2
variants1=( ${(@f)"$(printf '%s\n' $variants_yaml | _call_program -l xkb-parse-yaml yq ${(q)yq_prog1})"} )
variants2=( ${(@f)"$(printf '%s\n' $variants_yaml | _call_program -l xkb-parse-yaml yq ${(q)yq_prog2})"} )

case $status in
	127 ) _message "xkb model completion requires yq" && return 127 ;;
	<1->) _message "error completing xkb variants" && return 1 ;;
esac

_describe -t xkb-model 'xkb model' variants1 variants2 "$@"
