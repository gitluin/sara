#!/bin/bash

#TAGS="I:II:III:IV:V:VI:VII:VIII:IX"
#OTAGS="$TAGS"
#STAGS="$TAGS"
TAGS="\ue836:\ue836:\ue836:\ue836:\ue836:\ue836:\ue836:\ue836:\ue836"
OTAGS="\ue837:\ue837:\ue837:\ue837:\ue837:\ue837:\ue837:\ue837:\ue837"
STAGS="\ue3a6:\ue3a6:\ue3a6:\ue3a6:\ue3a6:\ue3a6:\ue3a6:\ue3a6:\ue3a6"
NTAGS="$TAGS"
OCCFG="#CBC19C"
#OCCBG="#2F3537"
SELFG="#CBC19C"
#SELBG="#2F3537"
TAGDELIMF="   "
TAGDELIMB="$TAGDELIMF"
LTDELIMF="  "
LTDELIMB="$LTDELIMF"

# TODO: this works, but not view/toggleview
LTBUTTONSTART="%{A1:sarasock 'setlayout tile':}%{A3:sarasock 'setlayout monocle':}"
LTBUTTONEND="%{A}%{A}"

# This man is a god
# https://jonas-langlotz.de/2020/10/05/polybar-on-dwm
xprop -spy -root "SARA_MONSTATE_$MONITOR" 2>/dev/null | {
	while read line; do
		# SARA_MONSTATE_0 = "SONNNNNNN:T"
		MONLINE="$(echo $line | cut -d' ' -f3 | sed 's/\"//g')"
		# DeskState:LayoutSymbol
		DESKSTATE="$(echo $MONLINE | cut -d':' -f1)"
		LAYOUTSYM="$(echo $MONLINE | cut -d':' -f2)"

		# Generate call to insert correct tags by using line numbers
		TAGSTR="$(echo $DESKSTATE | \
			sed 's/\(.\)/\1\n/g' | \
			sed '/^$/d' | \
			cat -n | \
			sed "s/^     \([0-9]\).\(.*\)/{{%{A1:sarasock \'view \1\':}${TAGDELIMF}\$(echo $\\2TAGS \| cut -d":" -f\1)${TAGDELIMB}%{A}}}/g")"
			#sed "s/^     \([0-9]\).\(.*\)/{{%{A1:sarasock \'view \1\':}%{A3:sarasock \'toggleview \1\':}${TAGDELIMF}\$(echo $\\2TAGS \| cut -d":" -f\1)${TAGDELIMB}%{A}%{A}}}/g")"

		# Insert colors
		TAGSTR="$(echo "$TAGSTR" | sed 's/{{\([^}}]*STAGS[^}}]*\)}}/%{F$SELFG}%{B$SELBG}{{\1}}%{F-}%{B-}/g')"
		TAGSTR="$(echo "$TAGSTR" | sed 's/{{\([^}}]*OTAGS[^}}]*\)}}/%{F$OCCFG}%{B$OCCBG}{{\1}}%{F-}%{B-}/g')"
		TAGSTR="$(echo "$TAGSTR" | sed 's/{{\([^}}]*NTAGS[^}}]*\)}}/%{F-}%{B-}{{\1}}/g')"

		# Remove newlines
		TAGSTR="$(echo $TAGSTR | sed 's/}} {{//g' | sed 's/{{//g' | sed 's/}}//g')"

		# Perform cut operations
		TAGSTR="$(eval echo "$TAGSTR")"

		# Layout stuff
		if test "$LAYOUTSYM" = "T"; then
			LAYOUTSYM=""
		elif test "$LAYOUTSYM" = "M"; then
			LAYOUTSYM=""
		elif test "$LAYOUTSYM" = "F"; then
			LAYOUTSYM=""
		fi
		TAGSTR="${TAGSTR}${LTBUTTONSTART}${LTDELIMF}$LAYOUTSYM${LTDELIMB}${LTBUTTONEND}%{B$BARBG}%{F$BARFG}"

		echo -e "${TAGSTR}"
	done
}
