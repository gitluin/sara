#
# ~/.bash_profile
#

[[ -f ~/.bashrc ]] && . ~/.bashrc

export PATH=$PATH:"/ibin"

[[ -z $DISPLAY && $XDG_VTNR -eq 1 ]] && exec startx
