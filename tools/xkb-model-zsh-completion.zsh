#compdef -value-,XKB_DEFAULT_MODEL,-default-

local models_yaml
models_yaml="$(_call_program -l xkb-yaml xkbcli list)"

case $status in
	127 ) _message "xkb model completion requires xkbcli" && return 127 ;;
	<1->) _message "error listing xkb models" && return 1 ;;
esac

local yq_prog='.models[] | "\(.name):\(.description)"'
local -a models
models=( ${(@f)"$(printf '%s\n' $models_yaml | _call_program -l xkb-parse-yaml yq ${(q)yq_prog})"} )
case $status in
	127)
		local sed_models='/^models:/,/^$/p' sed_names='s/- name: //p' sed_descriptions='s/  description: //p'
		local models_sed="$(_call_program -l sed sed -n ${(q)sed_models} <<< $models_yaml)"
		local -a model_names=( ${(@f)"$(_call_program -l sed-names sed -n ${(q)sed_names} <<< $models_sed)"})
		local -a model_descriptions=( ${(@f)"$(_call_program -l sed-models sed -n ${(q)sed_descriptions} <<< $models_sed)"})
		printf -v models '%s:%s' ${model_names:^model_descriptions}
		;;

	<1->) _message "error completing xkb models" && return 1 ;;
esac

_describe -t xkb-model 'xkb model' models "$@"
