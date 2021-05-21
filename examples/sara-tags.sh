#!/bin/bash

# Characters are from IcoMoon

#TAGS="I:II:III:IV:V:VI:VII:VIII:IX"
#OTAGS="$TAGS"
#STAGS="$TAGS"
TAGS="\uea56:\uea56:\uea56:\uea56:\uea56:\uea56:\uea56:\uea56:\uea56"
OTAGS="\uea54:\uea54:\uea54:\uea54:\uea54:\uea54:\uea54:\uea54:\uea54"
STAGS="\uea55:\uea55:\uea55:\uea55:\uea55:\uea55:\uea55:\uea55:\uea55"
NTAGS="$TAGS"
OCCFG="#CBC19C"
SELFG="#CBC19C"
TAGDELIMF="   "
TAGDELIMB="$TAGDELIMF"
LTDELIMF="  "
LTDELIMB="$LTDELIMF"

LTBUTTONSTART="%{A1:sarasock setlayout tile:}%{A3:sarasock setlayout monocle:}"
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
		#	(have to subtract 1, though)
		TAGSTR="$(echo $DESKSTATE | \
			sed 's/\(.\)/\1\n/g' | \
			sed '/^$/d' | \
			cat -n | \
			sed "s/^     \([0-9]\).\(.*\)/{{%{A1:sarasock view \$((\1 - 1)):}%{A3:sarasock toggleview \$((\1 - 1)):}${TAGDELIMF}\$(echo $\\2TAGS \| cut -d":" -f\1)${TAGDELIMB}%{A}%{A}}}/g")"

		# Insert colors
		TAGSTR="$(echo "$TAGSTR" | sed 's/{{\([^}}]*STAGS[^}}]*\)}}/%{F$SELFG}%{B$SELBG}{{\1}}%{F-}%{B-}/g')"
		TAGSTR="$(echo "$TAGSTR" | sed 's/{{\([^}}]*OTAGS[^}}]*\)}}/%{F$OCCFG}%{B$OCCBG}{{\1}}%{F-}%{B-}/g')"
		TAGSTR="$(echo "$TAGSTR" | sed 's/{{\([^}}]*NTAGS[^}}]*\)}}/%{F-}%{B-}{{\1}}/g')"

		# Remove newlines
		TAGSTR="$(echo $TAGSTR | sed 's/}} {{//g' | sed 's/{{//g' | sed 's/}}//g')"

		# Perform cut operations, set correct view numbers
		TAGSTR="$(eval echo "$TAGSTR")"

		# Layout stuff
		if test "$LAYOUTSYM" = "T"; then
			LAYOUTSYM="\ue964"
		elif test "$LAYOUTSYM" = "M"; then
			LAYOUTSYM="\ue91f"
		elif test "$LAYOUTSYM" = "F"; then
			LAYOUTSYM="\ue9c1"
		fi
		TAGSTR="${TAGSTR}${LTBUTTONSTART}${LTDELIMF}$LAYOUTSYM${LTDELIMB}${LTBUTTONEND}%{B$BARBG}%{F$BARFG}"

		echo -e "${TAGSTR}"
	done
}
