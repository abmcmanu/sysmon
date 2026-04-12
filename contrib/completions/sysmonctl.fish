# fish completion: copy to ~/.config/fish/completions/
set -l cmds ping version help list get history avg stats json alerts reload top

complete -c sysmonctl -f
complete -c sysmonctl -s s -r -F -d 'control socket'
complete -c sysmonctl -s w -x -d 'repeat every SEC seconds'
complete -c sysmonctl -n "not __fish_seen_subcommand_from $cmds" -a "$cmds"
complete -c sysmonctl -n "__fish_seen_subcommand_from get history avg stats" \
    -a "(sysmonctl list 2>/dev/null | string split -f1 ' ')"
