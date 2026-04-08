# bash completion for sysmonctl
# source it, or copy it to /usr/local/share/bash-completion/completions/sysmonctl

_sysmonctl() {
    local cur prev cmds
    cur=${COMP_WORDS[COMP_CWORD]}
    prev=${COMP_WORDS[COMP_CWORD - 1]}
    cmds="ping version help list get history avg stats json alerts reload top"

    case $prev in
        get | history | avg | stats)
            mapfile -t COMPREPLY < <(compgen -W "$(sysmonctl list 2> /dev/null | cut -d' ' -f1)" -- "$cur")
            return
            ;;
    esac
    mapfile -t COMPREPLY < <(compgen -W "$cmds" -- "$cur")
}
complete -F _sysmonctl sysmonctl
